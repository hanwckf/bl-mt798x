/*
 * Copyright (C) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <string.h>
#include <mbedtls/platform_util.h>
#include <mtk_efuse.h>
#include <mtk_crypto.h>
#include "mbedtls_helper.h"
#include "mtk_huk.h"

uint32_t mtk_get_derk_info(uint8_t *key_ptr, size_t *key_len, size_t size,
			   const uint8_t *salt, size_t salt_len,
			   const uint8_t *info, size_t info_len)
{
	uint32_t ret = 0;
	size_t len = 0;
	uint8_t hwk[MTK_HUK_KEY_LEN] = {0};
	uint8_t derk[MTK_HUK_DER_KEY_LEN] = {0};

	if (!key_ptr || !key_len || size < MTK_HUK_DER_KEY_LEN ||
	    !salt || !salt_len || !info || !info_len)
		return MTK_HUK_ERR_INVAL;

	ret = mtk_get_huk_info(hwk, &len, sizeof(hwk));
	if (ret)
		goto mtk_get_derk_info_err;

	ret = hkdf_derive_key(hwk, len, salt, salt_len,
			      info, info_len, derk, MTK_HUK_DER_KEY_LEN);
	if (ret) {
		ERROR("MTK-HUK: Derive DERK failed: %u\n", ret);
		ret = MTK_HUK_ERR_DERIVED;
		goto mtk_get_derk_info_err;
	}

	memcpy(key_ptr, derk, MTK_HUK_DER_KEY_LEN);
	*key_len = MTK_HUK_DER_KEY_LEN;

mtk_get_derk_info_err:
	mbedtls_platform_zeroize(derk, sizeof(derk));
	mbedtls_platform_zeroize(hwk, sizeof(hwk));

	return ret;
}

static uint32_t get_huid_info(uint8_t *huid_ptr, size_t *huid_len)
{
	uint32_t ret = 0;
	int i = 0;
	size_t len = 0;
	uint8_t huid_field[4] = {0};

	if (!huid_ptr || !huid_len)
		return MTK_HUK_ERR_INVAL;

	*huid_len = 0;

	for (i = 0; i < MTK_HUK_HUID_FIELDS; i++) {
		ret = mtk_efuse_read(MTK_EFUSE_FIELD_HUID0 + i,
				     huid_field, sizeof(huid_field));
		if (ret) {
			ERROR("MTK-HUK: EFUSE Read HUID failed: %u\n", ret);
			goto get_huid_info_err;
		}

		memcpy(huid_ptr + len, huid_field, sizeof(huid_field));

		len += sizeof(huid_field);
	}

	if (len != MTK_HUK_HUID_LEN) {
		ERROR("MTK-HUK: Wrong HUID length\n");
		goto get_huid_info_err;
	}

	*huid_len = MTK_HUK_HUID_LEN;

	for (i = 0; i < MTK_HUK_HUID_FIELDS; i++) {
		VERBOSE("MTK-HUK: EFUSE HUID%d field=%08x\n", i,
			*((uint32_t *)huid_ptr + i));
	}

	return 0;

get_huid_info_err:
	mbedtls_platform_zeroize(huid_ptr, MTK_HUK_HUID_LEN);

	return ret;
}

uint32_t mtk_get_huk_info(uint8_t *key_ptr, size_t *key_len, size_t size)
{
	uint32_t ret = 0;
	size_t huid_len = 0;
	uint8_t huid[MTK_EFUSE_MAX_HUID_LEN] = {0};
	uint8_t huk[MTK_HUK_KEY_LEN] __attribute__((aligned(16))) = {0};

	if (!key_ptr || !key_len || size < MTK_HUK_KEY_LEN)
		return MTK_HUK_ERR_INVAL;

	*key_len = 0;

	ret = get_huid_info(huid, &huid_len);
	if (ret) {
		ERROR("MTK-HUK: Get HUID failed: %u\n", ret);
		ret = MTK_HUK_ERR_EFUSE;
		goto mtk_get_huk_info_err;
	}

	ret = sej_encrypt(huid, huk, MTK_EFUSE_MAX_HUID_LEN);
	if (ret) {
		ERROR("MTK-HUK: Generate HUK failed: %u\n", ret);
		ret = MTK_HUK_ERR_SEJ;
		goto mtk_get_huk_info_err;
	}

	memcpy(key_ptr, huk, MTK_HUK_KEY_LEN);
	*key_len = MTK_HUK_KEY_LEN;

mtk_get_huk_info_err:
	mbedtls_platform_zeroize(huid, sizeof(huid));
	mbedtls_platform_zeroize(huk, sizeof(huk));

	return ret;
}
