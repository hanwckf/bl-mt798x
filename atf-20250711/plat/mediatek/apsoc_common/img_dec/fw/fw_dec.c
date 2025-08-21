// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <common/debug.h>
#include <lib/spinlock.h>
#include <mbedtls_helper.h>
#include <shm.h>
#include <mtk_roe.h>
#include <key_info.h>
#include <salt.h>
#include "fw_dec.h"

static spinlock_t fw_dec_lock;

static uint8_t fw_key[FW_KEY_SIZE];
static uint8_t kernel_key[FW_KEY_SIZE];
static uint8_t rootfs_key[FW_KEY_SIZE];

static int derive_fw_key(uint32_t key_idx, uint8_t *key, size_t key_len)
{
	int ret = 0;
	uint8_t *salt = NULL;
	uint8_t kernel_salt[] = KERNEL_KEY_SALT;
	uint8_t rootfs_salt[] = ROOTFS_KEY_SALT;

	switch (key_idx) {
	case KERNEL_KEY_IDX:
		salt = kernel_salt;
		break;
	case ROOTFS_KEY_IDX:
		salt = rootfs_salt;
		break;
	default:
		return -FW_DEC_KEY_IDX_ERR;
	}

	bl31_mbedtls_init();

	ret = derive_from_roe_key(salt, SALT_SIZE, key, key_len);
	if (ret)
		ERROR("%s: derive key failed: %d\n", __func__, ret);

	bl31_mbedtls_deinit();

	return ret;
}

void fw_dec_init(void)
{
	int ret;

	ret = derive_fw_key(KERNEL_KEY_IDX, kernel_key, sizeof(kernel_key));
	if (ret)
		panic();

	ret = derive_fw_key(ROOTFS_KEY_IDX, rootfs_key, sizeof(rootfs_key));
	if (ret)
		panic();
}

static int set_fw_key(uint32_t key_idx)
{
	uint8_t *key = NULL;

	switch (key_idx) {
	case KERNEL_KEY_IDX:
		key = kernel_key;
		break;
	case ROOTFS_KEY_IDX:
		key = rootfs_key;
		break;
	default:
		return -FW_DEC_KEY_IDX_ERR;
	}

	memcpy(fw_key, key, FW_KEY_SIZE);

	return 0;
}

#ifdef MTK_FW_ENC_VIA_BL31
static uint8_t iv[IV_SIZE];
static uint8_t iv_flag;
static uint8_t key_flag;

int fw_dec_set_key(uint32_t key_idx)
{
	int ret = 0;

	spin_lock(&fw_dec_lock);

	ret = set_fw_key(key_idx);
	if (ret)
		goto out;

	key_flag = 1;

out:
	spin_unlock(&fw_dec_lock);

	return ret;
}

int fw_dec_set_iv(uintptr_t iv_paddr, uint32_t iv_size)
{
	uintptr_t iv_vaddr;
	int ret = 0;
	int stat;

	if (!iv_paddr || iv_size != IV_SIZE)
		return -FW_DEC_INVALID_PARAM_ERR;

	ret = set_shared_memory(iv_paddr, iv_size, &iv_vaddr,
				MT_MEMORY | MT_RO | MT_NS);
	if (ret) {
		ERROR("%s: set_shared_memory failed\n", __func__);
		return ret;
	}

	spin_lock(&fw_dec_lock);

	memcpy(iv, (uint32_t *)iv_vaddr, iv_size);
	iv_flag = 1;

	spin_unlock(&fw_dec_lock);

	stat = free_shared_memory(iv_vaddr, iv_size);
	if (stat) {
		ERROR("%s: free_shared_memory failed\n", __func__);
		ret |= stat;
	}

	return ret;
}

static int do_decrypt(uint8_t *cipher, uint32_t cipher_size,
		      uint8_t *plain, uint32_t plain_size)
{
	int ret = 0;

	bl31_mbedtls_init();

	ret = aes_cbc_crypt(cipher, cipher_size,
			    fw_key, FW_KEY_SIZE,
			    iv, IV_SIZE,
			    plain, MBEDTLS_DECRYPT,
			    MBEDTLS_PADDING_NONE);
	if (ret) {
		ERROR("%s: image decryption failed: %d\n", __func__, ret);
		ret = -FW_DEC_IMAGE_DEC_ERR;
	}

	bl31_mbedtls_deinit();
	return ret;
}

int fw_dec_image(uintptr_t image_paddr, uint32_t image_size)
{
	uintptr_t cipher_vaddr, plain_vaddr;
	int ret = 0;
	int stat;

	if (!image_paddr || !image_size)
		return -FW_DEC_INVALID_PARAM_ERR;

	ret = set_shared_memory(image_paddr, image_size, &cipher_vaddr,
				MT_MEMORY | MT_RW | MT_NS);
	if (ret) {
		ERROR("%s: set_shared_memory failed\n", __func__);
		return ret;
	}

	spin_lock(&fw_dec_lock);

	if (!iv_flag || !key_flag) {
		ERROR("%s: decryption paramaters are not set\n", __func__);
		ret = -FW_DEC_PARAM_NOT_SET_ERR;
		goto out;
	}

	/* plain use same buffer with cipher */
	plain_vaddr = cipher_vaddr;

	ret = do_decrypt((uint8_t *)cipher_vaddr, image_size,
			 (uint8_t *)plain_vaddr, image_size);

out:
	memset(iv, 0, IV_SIZE);
	memset(fw_key, 0, FW_KEY_SIZE);
	iv_flag = 0;
	key_flag = 0;

	spin_unlock(&fw_dec_lock);

	stat = free_shared_memory(cipher_vaddr, image_size);
	if (stat) {
		ERROR("%s: free_shared_memory failed\n", __func__);
		ret |= stat;
	}
	return ret;
}
#endif /* MTK_FW_ENC_VIA_BL31 */

#ifdef MTK_FW_ENC_VIA_OPTEE
int fw_dec_get_key(uint32_t key_idx, uintptr_t paddr, uint32_t len)
{
	int ret = 0;
	int stat;
	uintptr_t vaddr;

	if (!paddr || len != FW_KEY_SIZE)
		return -FW_DEC_INVALID_PARAM_ERR;

#if defined(BL32_TZRAM_BASE) && defined(BL32_TZRAM_SIZE)
	if ((paddr & TABLE_ADDR_MASK) < BL32_TZRAM_BASE ||
	    (paddr & TABLE_ADDR_MASK) >= (BL32_TZRAM_BASE + BL32_TZRAM_SIZE - PAGE_SIZE))
		return -FW_DEC_INVALID_PARAM_ERR;
#else
	if ((paddr & TABLE_ADDR_MASK) < TZRAM2_BASE ||
	    (paddr & TABLE_ADDR_MASK) >= (TZRAM2_BASE + TZRAM2_SIZE - PAGE_SIZE))
		return -FW_DEC_INVALID_PARAM_ERR;
#endif

	ret = set_shared_memory(paddr, len, &vaddr,
				MT_MEMORY | MT_RW | MT_SECURE);
	if (ret) {
		ERROR("%s: set_shared_memory failed\n", __func__);
		return ret;
	}

	spin_lock(&fw_dec_lock);

	ret = set_fw_key(key_idx);
	if (ret)
		goto out;

	memcpy((uint8_t *)vaddr, fw_key, len);

	memset(fw_key, 0, FW_KEY_SIZE);

out:
	spin_unlock(&fw_dec_lock);

	stat = free_shared_memory(vaddr, len);
	if (stat) {
		ERROR("%s: free_shared_memory failed\n", __func__);
		ret |= stat;
	}

	return ret;
}
#endif /* MTK_FW_ENC_VIA_OPTEE */
