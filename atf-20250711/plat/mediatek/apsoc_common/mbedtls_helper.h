/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#ifndef _MBEDTLS_HELPER_H_
#define _MBEDTLS_HELPER_H_

#include <mbedtls/cipher.h>
#include <mbedtls/platform_util.h>

#define MTK_MBEDTLS_SUCC			U(0)
#define MTK_MBEDTLS_ERR_INVAL			U(1)
#define MTK_MBEDTLS_ERR_DEC			U(2)
#define MTK_MBEDTLS_ERR_CRYPT			U(3)
#define MTK_MBEDTLS_ERR_DERIVED			U(4)

uint32_t hkdf_derive_key(const uint8_t *key, size_t key_len,
			 const uint8_t *salt, size_t salt_len,
			 const uint8_t *info, size_t info_len,
			 uint8_t *out, size_t out_len);

uint32_t aes_cbc_crypt(const uint8_t *in, size_t in_len,
		       const uint8_t *key, size_t key_len,
		       const uint8_t *iv, size_t iv_len,
		       uint8_t *out, const mbedtls_operation_t oper,
		       const uint8_t padding_mode);

uint32_t aes_gcm_decrypt(const uint8_t *cipher, size_t cipher_len,
			 const uint8_t *key, size_t key_len,
			 const uint8_t *iv, size_t iv_len,
			 const uint8_t *tag, size_t tag_len,
			 const uint8_t *aad, size_t aad_len,
			 uint8_t *out);

void bl31_mbedtls_init(void);

void bl31_mbedtls_deinit(void);
#endif /* _MBEDTLS_HELPER_H_ */
