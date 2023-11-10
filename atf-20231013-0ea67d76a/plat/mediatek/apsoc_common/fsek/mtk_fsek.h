/*
 * Copyright (c) 2022, Mediatek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MTK_FSEK_H
#define MTK_FSEK_H

#include <common/runtime_svc.h>
#include <lib/mmio.h>

#define TMP_NS_SHM_BASE				(BL32_LIMIT)
#define TMP_NS_SHM_SIZE				(0x1000)

#define MTK_FSEK_RFS_KEY			"k-rootfs"
#define MTK_FSEK_DER_KEY			"k-dev-specific-derived"
#define MTK_FSEK_FIT_SECRET			"fit-secret"

#define MTK_FSEK_KEY_LEN			32
#define MTK_FSEK_FIT_SECRET_LEN			32
#define MTK_FSEK_MAX_DATA_LEN			160
#define MTK_FSEK_CBC_CIPHER_LEN			48
#define MTK_FSEK_GCM_CIPHER_LEN			32
#define MTK_FSEK_CBC_IV_LEN			16
#define MTK_FSEK_GCM_IV_LEN			12
#define MTK_FSEK_SALT_LEN			16
#define MTK_FSEK_TAG_LEN			16

#define MTK_FSEK_SUCC				U(0)
#define MTK_FSEK_ERR_WRONG_SRC			U(1)
#define MTK_FSEK_ERR_INVAL			U(2)
#define MTK_FSEK_ERR_MAP			U(3)
#define MTK_FSEK_ERR_DERIVED			U(4)
#define MTK_FSEK_ERR_DEC			U(5)
#define MTK_FSEK_ERR_VERIFY			U(6)
#define MTK_FSEK_ERR_MULTI_EXEC			U(7)
#define MTK_FSEK_ERR_KEY_UNK			U(8)
#define MTK_FSEK_ERR_ENC			U(9)
#define MTK_FSEK_ERR_HUK			U(11)
#define MTK_FSEK_ERR_UNMAP			U(32)

enum mtk_fsek_key_id {
	MTK_FSEK_RFS_KEY_ID = 0,
	MTK_FSEK_DER_KEY_ID,
	__MTK_FSEK_MAX_KEY_ID,
};
#define MTK_FSEK_MAX_KEY_ID (__MTK_FSEK_MAX_KEY_ID - 1)

struct fsek_dec_desc {
	const char	*cipher_name;
	uint8_t		*out;
	uint32_t	out_len;
	uint64_t (*verify)(uintptr_t hash);
};

struct fsek_tmpk_node {
	uint8_t		salt[MTK_FSEK_SALT_LEN];
	uint8_t		iv[MTK_FSEK_CBC_IV_LEN];
	const uint8_t	cipher[MTK_FSEK_CBC_CIPHER_LEN];
};

struct fsek_data_node {
	const uint8_t	iv[MTK_FSEK_GCM_IV_LEN];
	const uint8_t	tag[MTK_FSEK_TAG_LEN];
	const uint8_t	cipher[MTK_FSEK_GCM_CIPHER_LEN];
};

struct fsek_cipher_node {
	struct fsek_tmpk_node		tmpk;
	struct fsek_data_node		data;
};

#if MTK_FSEK
uint64_t mtk_fsek_get_shm_config(uintptr_t *paddr, size_t *size);

uint64_t mtk_fsek_decrypt_rfsk(void);

uint64_t mtk_fsek_get_key(const uint32_t key_id, void *key_ptr,
			  size_t size);

uint64_t mtk_fsek_encrypt_roek(u_register_t x1, u_register_t x2,
			       u_register_t x3, u_register_t x4,
			       void *key_ptr, size_t size);
#else
uint64_t mtk_fsek_get_shm_config(uintptr_t *paddr, size_t *size)
{
	return 0;
}

uint64_t mtk_fsek_decrypt_rfsk(void)
{
	return 0;
}

uint64_t mtk_fsek_get_key(const uint32_t key_id, void *key_ptr,
			  size_t size)
{
	return 0;
}

uint64_t mtk_fsek_encrypt_roek(u_register_t x1, u_register_t x2,
			       u_register_t x3, u_register_t x4,
			       void *key_ptr, size_t size)
{
	return 0;
}
#endif
#endif /* MTK_FSEK_H */
