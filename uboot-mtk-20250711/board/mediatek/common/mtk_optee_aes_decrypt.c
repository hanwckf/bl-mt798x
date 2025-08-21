// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: guan-gm.lin <guan-gm.lin@mediatek.com>
 */

#ifndef USE_HOSTCC
#include <tee.h>
#endif /* ifndef USE_HOSTCC */
#include <image.h>
#include <uboot_aes.h>

#ifndef USE_HOSTCC
#define TA_FIRMWARE_ENCRYPTION_UUID \
	{ 0x503810ea, 0x5f92, 0x49d3, \
		{ 0xa5, 0xf3, 0x87, 0xe9, 0xed, 0x02, 0x76, 0xa9 } }

enum {
	TZCMD_FW_ENC_SET_IV = 1,
	TZCMD_FW_ENC_SET_DATA,
	TZCMD_FW_ENC_SET_KEY
};

#define KERNEL_KEY_IDX			1
#define ROOTFS_KEY_IDX			2
#define SHM_SIZE			0x500000

static int session_init(struct udevice *tee, u32 *tee_session)
{
	struct tee_open_session_arg arg;
	struct tee_optee_ta_uuid uuid = TA_FIRMWARE_ENCRYPTION_UUID;
	int res;

	memset(&arg, 0, sizeof(arg));
	tee_optee_ta_uuid_to_octets(arg.uuid, &uuid);
	arg.clnt_login = TEE_LOGIN_PUBLIC;
	res = tee_open_session(tee, &arg, 0, NULL);
	if (res)
		return res;

	*tee_session = arg.session;

	return 0;
}

static int session_deinit(struct udevice *tee, u32 tee_session)
{
	int res;

	res = tee_close_session(tee, tee_session);
	if (res) {
		printf("tee_close_session failed: %x\n", res);
		return res;
	}

	return res;
}

static int set_iv(struct udevice *tee, u32 tee_session,
		  uint8_t *iv, uint32_t iv_len)
{
	int res;
	struct tee_invoke_arg arg;
	struct tee_param param[1];
	struct tee_shm *iv_shm = NULL;

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	res = tee_shm_register(tee, iv, iv_len, 0, &iv_shm);
	if (res) {
		printf("register IV share memory failed\n");
		goto out;
	}

	arg.func = TZCMD_FW_ENC_SET_IV;
	arg.session = tee_session;

	param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
	param[0].u.memref.shm = iv_shm;
	param[0].u.memref.size = iv_len;

	res = tee_invoke_func(tee, &arg, 1, param);
	if (res || arg.ret) {
		if (res) {
			printf("setup IV tee_invoke_func failed: %x\n", res);
			goto out;
		}
		res = arg.ret;
		printf("TEE: setup IV failed: %x\n", arg.ret);
		goto out;
	}

out:
	if (iv_shm != NULL)
		tee_shm_free(iv_shm);

	return res;
}

static int set_key(struct udevice *tee, u32 tee_session,
		   uint8_t key_idx)
{
	int res;
	struct tee_invoke_arg arg;
	struct tee_param param[1];

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	arg.func = TZCMD_FW_ENC_SET_KEY;
	arg.session = tee_session;

	param[0].attr = TEE_PARAM_ATTR_TYPE_VALUE_INPUT;
	param[0].u.value.a = key_idx;

	res = tee_invoke_func(tee, &arg, 1, param);
	if (res || arg.ret) {
		if (res) {
			printf("setup key tee_invoke_func failed: %x\n", res);
			return res;
		}
		res = arg.ret;
		printf("TEE: setup key failed: %x\n", arg.ret);
	}

	return res;
}

static int decrypt_image(struct udevice *tee, u32 tee_session,
			 uint8_t *cipher, size_t cipher_len,
			 uint8_t *plain, size_t plain_len)
{
	int res;
	struct tee_invoke_arg arg;
	struct tee_param param[2];
	struct tee_shm *cipher_shm = NULL;
	struct tee_shm *plain_shm = NULL;
	int dec_size = 0, shm_size = 0;

	memset(param, 0, sizeof(param));
	memset(&arg, 0, sizeof(arg));

	while (dec_size <= cipher_len) {
		shm_size = ((dec_size + SHM_SIZE) < cipher_len) ? SHM_SIZE : cipher_len % SHM_SIZE;

		res = tee_shm_register(tee, cipher, shm_size, 0, &cipher_shm);
		if(res) {
			printf("setup cipher data share memory failed\n");
			goto out;
		}

		res = tee_shm_register(tee, plain, shm_size, 0, &plain_shm);
		if (res) {
			printf("setup plain data share memory failed\n");
			goto out;
		}

		arg.func = TZCMD_FW_ENC_SET_DATA;
		arg.session = tee_session;

		param[0].attr = TEE_PARAM_ATTR_TYPE_MEMREF_INPUT;
		param[0].u.memref.shm = cipher_shm;
		param[0].u.memref.size = shm_size;
		param[1].attr = TEE_PARAM_ATTR_TYPE_MEMREF_OUTPUT;
		param[1].u.memref.shm = plain_shm;
		param[1].u.memref.size = shm_size;

		res = tee_invoke_func(tee, &arg, 2, param);
		if (res || arg.ret) {
			if (res) {
				printf("decrypt image tee_invoke_func failed: %x\n", res);
				goto out;
			}
			res = arg.ret;
			printf("TEE: decrypt image failed: %x\n", arg.ret);
			goto out;
		}

		if (cipher_shm != NULL) {
			tee_shm_free(cipher_shm);
			cipher_shm = NULL;
		}

		if (plain_shm != NULL) {
			tee_shm_free(plain_shm);
			plain_shm = NULL;
		}

		dec_size += SHM_SIZE;
		cipher += SHM_SIZE;
		plain += SHM_SIZE;
	}

out:
	if (cipher_shm != NULL)
		tee_shm_free(cipher_shm);

	if (plain_shm != NULL)
		tee_shm_free(plain_shm);

	return res;
}

static int image_decrypt_via_optee(struct udevice *tee, uint8_t key_idx,
				   uint8_t *iv, uint32_t iv_len,
				   uint8_t *cipher, size_t cipher_len,
				   uint8_t *plain, size_t plain_len)
{
	u32 tee_session = 0;
	int res;

	res = session_init(tee, &tee_session);
	if (res) {
		printf("session initialization failed: %d\n", res);
		return res;
	}

	res = set_key(tee, tee_session, key_idx);
	if (res) {
		printf("setup image decryption key failed: %d\n", res);
		goto out;
	}

	res = set_iv(tee, tee_session, iv, iv_len);
	if (res) {
		printf("setup image decryption IV failed: %d\n", res);
		goto out;
	}

	res = decrypt_image(tee, tee_session, cipher, cipher_len,
			    plain, plain_len);
	if (res) {
		printf("decrypt image failed: %d\n", res);
		goto out;
	}

out:
	session_deinit(tee, tee_session);

	return res;
}
#endif /* ifndef USE_HOSTCC */

int mtk_optee_image_aes_decrypt(struct image_cipher_info *info,
				const void *cipher, size_t cipher_len,
				void **data, size_t *size)
{
#ifndef USE_HOSTCC
	struct udevice *tee;
	uint32_t iv_len;
	uint8_t *iv;
	uint8_t key_idx = 0;

	if (!strncmp(info->keyname, "kernel_key", 10)) {
		key_idx = KERNEL_KEY_IDX;
	} else if (!strncmp(info->keyname, "rootfs_key", 10)) {
		key_idx = ROOTFS_KEY_IDX;
	} else {
		printf("cannot find key index for keyname: %s\n", info->keyname);
		return -EINVAL;
	}

	iv = (uint8_t *)info->iv;
	iv_len = info->cipher->iv_len;

	*data = (void *)cipher;
	*size = info->size_unciphered;

	tee = tee_find_device(NULL, NULL, NULL, NULL);
	if (!tee) {
		printf("Can't find tee device\n");
		return -ENODEV;
	}

	if (image_decrypt_via_optee(tee, key_idx, iv, iv_len,
			(uint8_t *)cipher, cipher_len, *data, cipher_len)) {
		printf("image decryption via OP-TEE failed\n");
		return -EINVAL;
	}

#endif /* ifndef USE_HOSTCC */
	return 0;
}
