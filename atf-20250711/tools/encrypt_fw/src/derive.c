/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-CLause
 */

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <stdio.h>
#include <string.h>
#include "debug.h"
#include "derive.h"
#include <key_info.h>
#include <salt.h>

static int hex2bin(char *hex, int hex_len, uint8_t *out)
{
	int i, j;

	for (i = 0, j = 0; j < hex_len; i++, j += 2) {
		if (sscanf(&hex[j], "%02hhx", &out[i]) != 1) {
			ERROR("Incorrect key format\n");
			return -1;
		}
	}
	return 0;
}

static int bin2hex(uint8_t *bin, int bin_len, char *out)
{
	int i, j;

	for (i = 0, j = 0; i < bin_len; i++, j += 2) {
		if (sprintf(&out[j], "%02x", bin[i]) < 0) {
			ERROR("Incorrect key format\n");
			return -1;
		}
	}
	return 0;
}

static int do_hkdf(char *key_hex, uint32_t key_hex_len,
		   uint8_t *salt, uint32_t salt_len,
		   char *buf, size_t out_len)
{
	EVP_PKEY_CTX *pctx;
	uint8_t key[DERIVE_KEY_SIZE] = { 0 };
	uint8_t out_key[DERIVE_KEY_SIZE] = { 0 };
	uint32_t ret = 0;
	size_t len = sizeof(out_key);

	pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);
	if (!pctx) {
		ERROR("EVP_PKEY_CTX_new_id: error\n");
		return -1;
	}

	ret = hex2bin(key_hex, key_hex_len, key);
	if (ret) {
		ERROR("hex2bin key error\n");
		goto out;
	}

	ret = EVP_PKEY_derive_init(pctx);
	if (ret <= 0) {
		ERROR("EVP_PKEY_derive_init failed: %x\n", ret);
		goto out;
	}

	ret = EVP_PKEY_CTX_set_hkdf_md(pctx, EVP_sha256());
	if (ret <= 0) {
		ERROR("EVP_PKEY_CTX_set_hkdf_md failed: %x\n", ret);
		goto out;
	}

	ret = EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt, salt_len);
	if (ret <= 0) {
		ERROR("EVP_PKEY_CTX_set1_hdkf_salt failed: %x\n", ret);
		goto out;
	}

	ret = EVP_PKEY_CTX_set1_hkdf_key(pctx, key, key_hex_len / 2);
	if (ret <= 0) {
		ERROR("EVP_PKEY_CTX_set1_hkdf_key failed: %x\n", ret);
		goto out;
	}

	ret = EVP_PKEY_derive(pctx, out_key, &len);
	if (ret <= 0) {
		ERROR("EVP_PKEY_derive failed: %x\n", ret);
		goto out;
	}

	if (len * 2 >= out_len) {
		ERROR("output buffer length should be larger than %lu, but is %lu\n",
		      len, out_len);
		goto out;
	}

	ret = bin2hex(out_key, sizeof(out_key), buf);
	if (ret) {
		ERROR("bin2hex key error\n");
		goto out;
	}

out:
	EVP_PKEY_CTX_free(pctx);
	return ret;
}

int derive_enc_key(char *plat_key, uint32_t plat_key_len,
		   char *enc_key, uint32_t enc_key_size)
{
	int ret = 0;
	char roe_key[ROE_KEY_SIZE * 2 + 1] = { 0 };
	uint8_t roe_salt[] = ROE_KEY_SALT;
#ifdef FIP_KEY_SALT
	uint8_t fip_salt[] = FIP_KEY_SALT;
#else
	uint8_t fip_salt[] = { 0 };
#endif

	if (enc_key_size <= DERIVE_KEY_SIZE * 2)
		return -1;

	ret = do_hkdf(plat_key, plat_key_len, roe_salt, sizeof(roe_salt),
		      roe_key, sizeof(roe_key));
	if (ret) {
		ERROR("derive ROE key failed\n");
		return ret;
	}

	memset(enc_key, 0, enc_key_size);

	ret = do_hkdf(roe_key, strlen(roe_key), fip_salt, sizeof(fip_salt),
		      enc_key, enc_key_size);
	if (ret) {
		ERROR("derive encryption key failed\n");
		return ret;
	}

	return ret;
}
