/*
 * Copyright (c) 2022, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-CLause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/spinlock.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform_def.h>
#include <plat/common/platform.h>
#include <mbedtls/memory_buffer_alloc.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/cipher.h>
#include "mbedtls_helper.h"
#include "mtk_huk.h"
#include "mtk_roek.h"
#include "mtk_fsek.h"

static spinlock_t fsek_dec_rfsk_lock;
static spinlock_t fsek_get_key_lock;

static uint8_t rfsk[MTK_FSEK_KEY_LEN];
static uint8_t fit_secret[MTK_FSEK_FIT_SECRET_LEN];

static uint32_t salt_1[] = {
	0x06BACD1D, 0xC8567BB4, 0x91D9D66B, 0x3691228B,
	0xDBC09405, 0x29B8E672, 0xE1AED3C6, 0xDE2A20FD,
};

static const uintptr_t shm_paddr = TMP_NS_SHM_BASE;
static const size_t shm_size = TMP_NS_SHM_SIZE;

static uint64_t verify_fit_secret(uintptr_t sig_hash)
{
	uint8_t hash[MTK_FSEK_FIT_SECRET_LEN];

	if (!sig_hash)
		return MTK_FSEK_ERR_INVAL;

	memcpy(hash, (void *)sig_hash, MTK_FSEK_FIT_SECRET_LEN);

	if (memcmp(fit_secret, hash, MTK_FSEK_FIT_SECRET_LEN)) {
		ERROR("FSEK: Verify FIT secret failed\n");
		return MTK_FSEK_ERR_VERIFY;
	}

	return 0;
}

/* make sure out_len < MTK_FSEK_MAX_CIPHER_LEN */
static const struct fsek_dec_desc fsek_dec_descs[] = {
	[0] = {
		.cipher_name = MTK_FSEK_FIT_SECRET,
		.out = fit_secret,
		.out_len = sizeof(fit_secret),
		.verify = verify_fit_secret
	},
	[1] = {
		.cipher_name = MTK_FSEK_RFS_KEY,
		.out = rfsk,
		.out_len = sizeof(rfsk)
	}
};

static void mbedtls_init(void)
{
	int ret;
	void *heap_addr = NULL;
	size_t heap_size = 0;

	ret = plat_get_mbedtls_heap(&heap_addr, &heap_size);
	if (ret) {
		ERROR("[%s] MBEDTLS failed to get a heap\n", __func__);
		panic();
	}

	mbedtls_memory_buffer_alloc_init(heap_addr, heap_size);
}

static void mbedtls_deinit(void)
{
	mbedtls_memory_buffer_alloc_free();
}

uint64_t mtk_fsek_get_shm_config(uintptr_t *paddr, size_t *size)
{
	if (!paddr || !size)
		return MTK_FSEK_ERR_INVAL;

	*paddr = shm_paddr;
	*size = shm_size;

	return MTK_FSEK_SUCC;
}

static uint64_t decrypt_tmpk(struct fsek_tmpk_node *node,
			     const struct fsek_dec_desc *desc,
			     uint8_t *roek, size_t roek_len,
			     uint8_t *tmpk)
{
	uint64_t ret = 0;
	uint8_t key[MTK_FSEK_KEY_LEN] = {0};

	if (!node || !desc || !roek || !roek_len || !tmpk)
		return MTK_FSEK_ERR_INVAL;

	ret = hkdf_derive_key(roek, roek_len,
			      node->salt, MTK_FSEK_SALT_LEN,
			      /* use cipher name as info */
			      (uint8_t *)desc->cipher_name,
			      strlen(desc->cipher_name),
			      key, MTK_FSEK_KEY_LEN);
	if (ret) {
		ERROR("FSEK: Key derivation for %s failed\n",
		      desc->cipher_name);
		ret = MTK_FSEK_ERR_DERIVED;
		goto decrypt_tmpk_err;
	}

	ret = aes_cbc_crypt(node->cipher, MTK_FSEK_CBC_CIPHER_LEN,
			    key, MTK_FSEK_KEY_LEN,
			    node->iv, MTK_FSEK_CBC_IV_LEN,
			    tmpk, MBEDTLS_DECRYPT);
	if (ret) {
		ERROR("FSEK: Decrypt TMPK for %s failed\n",
		      desc->cipher_name);
		ret = MTK_FSEK_ERR_DEC;
	}

decrypt_tmpk_err:
	mbedtls_platform_zeroize(key, sizeof(key));

	return ret;
}

static uint64_t fsek_decrypt(struct fsek_cipher_node *cipher_node,
			     const struct fsek_dec_desc *desc,
			     uint8_t *roek, size_t roek_len)
{
	uint64_t ret = 0;
	struct fsek_data_node *node = NULL;
	uint8_t tmpk[MTK_FSEK_CBC_CIPHER_LEN + 16] = {0};

	if (!cipher_node || !desc || !roek || !roek_len)
		return MTK_FSEK_ERR_INVAL;

	ret = decrypt_tmpk(&cipher_node->tmpk, desc,
			   roek, roek_len, tmpk);
	if (ret)
		goto fsek_decrypt_err;

	node = &cipher_node->data;

	ret = aes_gcm_decrypt(node->cipher, desc->out_len,
			      tmpk, MTK_FSEK_KEY_LEN,
			      node->iv, MTK_FSEK_GCM_IV_LEN,
			      node->tag, MTK_FSEK_TAG_LEN,
			      /* use tmpk-x cipher node as aad */
			      (uint8_t *)&cipher_node->tmpk,
			      sizeof(cipher_node->tmpk),
			      desc->out);
	if (ret) {
		ERROR("FSEK: Decrypt %s failed\n", desc->cipher_name);
		ret = MTK_FSEK_ERR_DEC;
	}

fsek_decrypt_err:
	mbedtls_platform_zeroize(tmpk, sizeof(tmpk));

	return ret;
}

/*
 * fsek_decrypt_rfsk
 * @shm_vaddr: virtual shared memory address
 * Decrypt rootfs-key, verify fit-secret.
 *
 * For now, we assume shm is in following format:
 * for each node is fixed size, MTK_FSEK_MAX_DATA_LEN.
 * Low address	------------------------------------------------------
 *		| roe-key.enc                                        |
 *		------------------------------------------------------
 *		| salt | iv | k-temp2.enc | iv | tag | fit-secret    |
 *		------------------------------------------------------
 *		| salt | iv | k-temp1.enc | iv | tag | k-rootfs.enc  |
 *		------------------------------------------------------
 *		| fit-signature's hash   |                           |
 * High address	------------------------------------------------------
 *
 * returns:
 *	0: on success
 *	MTK_FSEK_ERR_DERIVED: key derivation failed
 *	MTK_FSEK_ERR_DEC: decryption failed
 *	MTK_FSEK_ERR_VERIFY: verifying fit-secret failed
 */
static uint64_t fsek_decrypt_rfsk(uintptr_t shm_vaddr)
{
	uint64_t ret = 0;
	int i = 0;
	uintptr_t p = 0;
	uintptr_t hash_p = 0;
	size_t roek_len = 0;
	struct fsek_cipher_node cipher_node = {0};
	uint8_t roek[MTK_FSEK_KEY_LEN] = {0};

	if (!shm_vaddr)
		return MTK_FSEK_ERR_INVAL;

	ret = mtk_get_roek_info_from_extern(roek, &roek_len,
					    MTK_FSEK_KEY_LEN,
					    (void *)shm_vaddr);
	if (ret) {
		ret = MTK_FSEK_ERR_DEC;
		goto fsek_decrypt_rfsk_err;
	}

	p = shm_vaddr + MTK_FSEK_MAX_DATA_LEN;
	hash_p = p + ARRAY_SIZE(fsek_dec_descs) * MTK_FSEK_MAX_DATA_LEN;

	for (i = 0; i < ARRAY_SIZE(fsek_dec_descs); i++) {
		memcpy(&cipher_node, (void *)p, sizeof(cipher_node));

		ret = fsek_decrypt(&cipher_node, &fsek_dec_descs[i],
				   roek, roek_len);
		if (ret)
			goto fsek_decrypt_rfsk_err;

		if (fsek_dec_descs[i].verify) {
			ret = fsek_dec_descs[i].verify(hash_p);
			if (ret)
				goto fsek_decrypt_rfsk_err;
		}

		p += MTK_FSEK_MAX_DATA_LEN;
	}

	mbedtls_platform_zeroize(roek, sizeof(roek));

	return 0;

fsek_decrypt_rfsk_err:
	for (i = 0; i < ARRAY_SIZE(fsek_dec_descs); i++) {
		mbedtls_platform_zeroize(fsek_dec_descs[i].out,
					 fsek_dec_descs[i].out_len);
	}

	mbedtls_platform_zeroize(roek, sizeof(roek));

	return ret;
}

uint64_t mtk_fsek_decrypt_rfsk(void)
{
	uint64_t ret = 0;
	int stat = 0;
	static bool dec_rfsk_exec;
	uintptr_t shm_vaddr = 0;

	spin_lock(&fsek_dec_rfsk_lock);

	if (dec_rfsk_exec) {
		ret = MTK_FSEK_ERR_MULTI_EXEC;
		goto mtk_fsek_decrypt_rfsk_exit;
	}
	dec_rfsk_exec = true;

	stat = mmap_add_dynamic_region_alloc_va(shm_paddr & TABLE_ADDR_MASK,
						&shm_vaddr, shm_size,
						MT_MEMORY | MT_RO | MT_NS);
	if (stat) {
		ERROR("FSEK: Mapping region failed: %d\n", stat);
		ret = MTK_FSEK_ERR_MAP;
		goto mtk_fsek_decrypt_rfsk_exit;
	}

	mbedtls_init();

	ret = fsek_decrypt_rfsk(shm_vaddr);

	mbedtls_deinit();

	stat = mmap_remove_dynamic_region(shm_vaddr, shm_size);
	if (stat) {
		ERROR("FSEK: Unmapping region failed: %d\n", stat);
		ret |= MTK_FSEK_ERR_UNMAP;
	}

mtk_fsek_decrypt_rfsk_exit:
	spin_unlock(&fsek_dec_rfsk_lock);

	return ret;
}

uint64_t mtk_fsek_get_key(const uint32_t key_id, void *key_ptr,
			  size_t size)
{
	uint64_t ret = 0;
	size_t len = 0;
	void *key = NULL;
	uint8_t derk[MTK_FSEK_KEY_LEN] = {0};

	if (key_id > MTK_FSEK_MAX_KEY_ID || !key_ptr ||
		size < MTK_FSEK_KEY_LEN)
		return MTK_FSEK_ERR_INVAL;

	spin_lock(&fsek_get_key_lock);

	mbedtls_init();

	switch (key_id) {
	case MTK_FSEK_RFS_KEY_ID:
		key = rfsk;
		break;
	case MTK_FSEK_DER_KEY_ID:
		ret = mtk_get_derk_info(derk, &len, sizeof(derk),
					(uint8_t *)salt_1, sizeof(salt_1),
					/* use derk str as info */
					(uint8_t *)MTK_FSEK_DER_KEY,
					strlen(MTK_FSEK_DER_KEY));
		key = (ret) ? NULL : derk;
		break;
	default:
		key = NULL;
		break;
	}

	if (!key)
		goto mtk_fsek_get_key_exit;

	memcpy(key_ptr, key, MTK_FSEK_KEY_LEN);
	mbedtls_platform_zeroize(key, MTK_FSEK_KEY_LEN);

mtk_fsek_get_key_exit:
	mbedtls_deinit();

	spin_unlock(&fsek_get_key_lock);

	return 0;
}

uint64_t mtk_fsek_encrypt_roek(u_register_t x1, u_register_t x2,
			       u_register_t x3, u_register_t x4,
			       void *key_ptr, size_t size)
{
	uint64_t ret = 0;

	mbedtls_init();

	ret = mtk_roek_encrypt(x1, x2, x3, x4, key_ptr, size);

	mbedtls_deinit();

	return ret;
}
