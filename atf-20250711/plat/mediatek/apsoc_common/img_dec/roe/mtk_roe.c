// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <string.h>
#include <common/debug.h>
#include <mbedtls_helper.h>
#include <key_info.h>
#include <salt.h>
#include "mtk_plat_key.h"
#include "mtk_roe.h"

static uint8_t roe_key[ROE_KEY_SIZE];

int check_key_is_zero(uint8_t *key, size_t key_len)
{
	uint32_t i;

	if (key == NULL || key_len == 0)
		return true;

	for (i = 0; i < key_len; i++) {
		if (key[i])
			return false;
	}

	return true;
}

static int get_roe_key(void)
{
	int ret = 0;
	uint8_t info[] = "";
	uint8_t salt[] = ROE_KEY_SALT;
	uint8_t plat_key[PLAT_KEY_SIZE] = { 0 };

	if (!check_key_is_zero(roe_key, ROE_KEY_SIZE))
		goto out;

	ret = get_plat_key(plat_key, sizeof(plat_key));
	if (ret)
		return ret;

	ret = hkdf_derive_key(plat_key, sizeof(plat_key), salt, sizeof(salt),
			      info, 0, roe_key, ROE_KEY_SIZE);
	if (ret) {
		memset(roe_key, 0, ROE_KEY_SIZE);
		goto out;
	}

	if (check_key_is_zero(roe_key, ROE_KEY_SIZE)) {
		ERROR("%s: invalid derived key\n", __func__);
		ret = -ROE_DERIVE_KEY_ERR;
	}

out:
	mbedtls_platform_zeroize(plat_key, PLAT_KEY_SIZE);
	return ret;
}

int set_roe_key_next_bl_params(uint64_t *key_arg, uint64_t *len_arg)
{
	int ret;

	ret = get_roe_key();
	if (ret)
		return ret;

	/* make sure remaining roe key is only in one place */
	*key_arg = (uint64_t)roe_key;
	*len_arg = ROE_KEY_SIZE;

	return 0;
}

int init_roe_key(uint8_t *key, size_t key_len)
{
	int ret = 0;

	if (key == NULL)
		return -ROE_INVALID_PARAM_ERR;

	if (key_len != ROE_KEY_SIZE) {
		ret = -ROE_INVALID_PARAM_ERR;
		goto err;
	}

	if (check_key_is_zero(key, ROE_KEY_SIZE)) {
		ERROR("%s: invalid key parameter\n", __func__);
		ret = -ROE_INIT_KEY_ERR;
		goto err;
	}

	if (!check_key_is_zero(roe_key, ROE_KEY_SIZE)) {
		ERROR("%s: key already initialized\n", __func__);
		ret = -ROE_INIT_KEY_ERR;
		goto err;
	}

	memcpy(roe_key, key, ROE_KEY_SIZE);
	memset(key, 0, key_len);

	return 0;

err:
	memset(roe_key, 0, ROE_KEY_SIZE);
	memset(key, 0, key_len);
	return ret;
}

void reset_roe_key(void)
{
	memset(roe_key, 0, ROE_KEY_SIZE);
}

int derive_from_roe_key(uint8_t *salt, size_t salt_len,
			uint8_t *key, size_t key_len)
{
	int ret = 0;
	uint8_t info[] = "";

	if (key == NULL || key_len != ROE_KEY_SIZE)
		return -ROE_INVALID_PARAM_ERR;

	if (salt == NULL || salt_len != SALT_SIZE)
		return -ROE_INVALID_PARAM_ERR;

	ret = get_roe_key();
	if (ret) {
		ERROR("%s: get ROE key failed: %d\n", __func__, ret);
		return ret;
	}

	ret = hkdf_derive_key(roe_key, ROE_KEY_SIZE, salt, salt_len,
			      info, 0, key, key_len);
	if (ret) {
		memset(key, 0, key_len);
		goto err;
	}

	if (check_key_is_zero(key, key_len)) {
		ERROR("%s: invalid derived key\n", __func__);
		ret = -ROE_DERIVE_KEY_ERR;
	}

err:
	return ret;
}
