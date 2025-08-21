// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: guan-gm.lin <guan-gm.lin@mediatek.com>
 */

#ifndef USE_HOSTCC
#include <linux/arm-smccc.h>
#endif /* ifndef USE_HOSTCC */
#include <image.h>
#include <uboot_aes.h>

#ifndef USE_HOSTCC

#define MTK_SIP_FW_DEC_SET_IV			0xC2000580
#define MTK_SIP_FW_DEC_SET_KEY			0xC2000581
#define MTK_SIP_FW_DEC_IMAGE			0xC2000582

#define KERNEL_KEY_IDX				1
#define ROOTFS_KEY_IDX				2

static int set_iv(uint8_t *iv, uint32_t iv_len)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(MTK_SIP_FW_DEC_SET_IV, (uintptr_t)iv, iv_len, 0, 0, 0, 0, 0, &res);

	return res.a0;
}


static int set_key(uint8_t key_idx)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(MTK_SIP_FW_DEC_SET_KEY, key_idx, 0, 0, 0, 0, 0, 0, &res);

	return res.a0;
}

static int image_decrypt(uint8_t *cipher, size_t cipher_len,
			 uint8_t *plain, size_t plain_len)
{
	struct arm_smccc_res res = { 0 };

	arm_smccc_smc(MTK_SIP_FW_DEC_IMAGE, (uintptr_t)cipher, cipher_len,
		      (uintptr_t)plain, plain_len, 0, 0, 0, &res);

	return res.a0;
}

static int image_decrypt_via_smc(uint8_t key_idx, uint8_t *iv, uint32_t iv_len,
				 uint8_t *cipher, size_t cipher_len,
				 uint8_t *plain, size_t plain_len)
{
	int res;

	res = set_key(key_idx);
	if (res) {
		printf("setup image decryption key failed: %d\n", res);
		goto out;
	}

	res = set_iv(iv, iv_len);
	if (res) {
		printf("setup image decryption IV failed: %d\n", res);
		goto out;
	}

	res = image_decrypt(cipher, cipher_len, plain, plain_len);
	if (res) {
		printf("image decryption failed: %d\n", res);
		goto out;
	}

out:
	return res;
}
#endif /* ifndef USE_HOSTCC */

int mtk_image_aes_decrypt(struct image_cipher_info *info,
			  const void *cipher, size_t cipher_len,
			  void **data, size_t *size)
{
#ifndef USE_HOSTCC
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

	/* use same buffer in cipher and plain*/
	*data = (void *)cipher;
	*size = info->size_unciphered;

	if (image_decrypt_via_smc(key_idx, iv, iv_len,
			(uint8_t *)cipher, cipher_len, *data, cipher_len)) {
		printf("image decryption via SMC call failed\n");
		return -EINVAL;
	}

#endif /* ifndef USE_HOSTCC */
	return 0;
}
