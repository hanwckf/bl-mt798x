/*
 * Copyright (c) 2022, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-CLause
 */

#include <common/debug.h>
#include <plat/common/platform.h>
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/gcm.h>
#include <mbedtls/platform_util.h>
#include "mbedtls_helper.h"

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}

uint32_t hkdf_derive_key(const uint8_t *key, size_t key_len,
			 const uint8_t *salt, size_t salt_len,
			 const uint8_t *info, size_t info_len,
			 uint8_t *out, size_t out_len)
{
	int ret = 0;
	const mbedtls_md_info_t *md_info = NULL;

	if (!key || !key_len || !salt || !salt_len ||
		!info || !info_len || !out || !out_len)
		return MTK_MBEDTLS_ERR_INVAL;

	md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!md_info) {
		ERROR("MBEDTLS: Get MD info failed\n");
		return MTK_MBEDTLS_ERR_DERIVED;
	}

	ret = mbedtls_hkdf(md_info, salt, salt_len, key, key_len,
			   info, info_len, out, out_len);
	if (ret)
		ERROR("MBEDTLS: HKDF key derivation failed: %d\n", ret);

	return (ret) ? MTK_MBEDTLS_ERR_DERIVED : 0;
}

uint32_t aes_cbc_crypt(const uint8_t *in, size_t in_len,
		       const uint8_t *key, size_t key_len,
		       const uint8_t *iv, size_t iv_len,
		       uint8_t *out, const mbedtls_operation_t oper)
{
	int ret = 0;
	size_t len = 0;
	size_t olen = 0;
	mbedtls_cipher_context_t aes_ctx = {0};
	const mbedtls_cipher_info_t *info = NULL;

	if (!in || !in_len || !key || !key_len || !iv ||
		!iv_len || !out)
		return MTK_MBEDTLS_ERR_INVAL;

	info = mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_256_CBC);
	if (!info) {
		ERROR("MBEDTLS: Get cipher info failed\n");
		return MTK_MBEDTLS_ERR_CRYPT;
	}

	mbedtls_cipher_init(&aes_ctx);

	ret = mbedtls_cipher_setup(&aes_ctx, info);
	if (ret) {
		ERROR("MBEDTLS: Cipher setup failed: %d\n", ret);
		goto aes_cbc_crypt_err;
	}

	ret = mbedtls_cipher_setkey(&aes_ctx, key,
				    key_len << 3, oper);
	if (ret) {
		ERROR("MBEDTLS: Cipher set key failed: %d\n", ret);
		goto aes_cbc_crypt_err;
	}

	ret = mbedtls_cipher_set_iv(&aes_ctx, iv, iv_len);
	if (ret) {
		ERROR("MBEDTLS: Cipher set iv failed: %d\n", ret);
		goto aes_cbc_crypt_err;
	}

	ret = mbedtls_cipher_update(&aes_ctx, in, in_len, out, &len);
	if (ret) {
		ERROR("MBEDTLS: Cipher update failed: %d\n", ret);
		goto aes_cbc_crypt_err;
	}
	olen += len;

	ret = mbedtls_cipher_finish(&aes_ctx, out, &len);
	if (ret)
		ERROR("MBEDTLS: Cipher finish failed: %d\n", ret);
	olen += len;

aes_cbc_crypt_err:
	mbedtls_cipher_free(&aes_ctx);

	return (ret) ? MTK_MBEDTLS_ERR_CRYPT : 0;
}

uint32_t aes_gcm_decrypt(const uint8_t *cipher, size_t cipher_len,
			 const uint8_t *key, size_t key_len,
			 const uint8_t *iv, size_t iv_len,
			 const uint8_t *tag, size_t tag_len,
			 const uint8_t *aad, size_t aad_len,
			 uint8_t *out)
{
	int ret = 0;
	mbedtls_gcm_context gcm_ctx = {0};

	if (!cipher || !cipher_len || !key || !key_len || !iv || !iv_len
	    || !tag || !tag_len || !aad || !aad_len || !out)
		return MTK_MBEDTLS_ERR_INVAL;

	mbedtls_gcm_init(&gcm_ctx);

	ret = mbedtls_gcm_setkey(&gcm_ctx, MBEDTLS_CIPHER_ID_AES,
				 key, (key_len << 3));
	if (ret) {
		ERROR("MBEDTLS: GCM set key failed: %d\n", ret);
		goto aes_gcm_decrypt_err;
	}

	ret = mbedtls_gcm_auth_decrypt(&gcm_ctx, cipher_len, iv, iv_len,
				       aad, aad_len, tag, tag_len,
				       cipher, out);
	if (ret)
		ERROR("MBEDTLS: GCM Decryption failed: %d\n", ret);

aes_gcm_decrypt_err:
	mbedtls_gcm_free(&gcm_ctx);

	return (ret) ? MTK_MBEDTLS_ERR_DEC : 0;
}
