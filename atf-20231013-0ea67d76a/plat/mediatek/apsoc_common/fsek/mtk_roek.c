/*
 * Copyright (C) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <string.h>
#include <mbedtls/platform_util.h>
#include <mbedtls/cipher.h>
#include <mtk_efuse.h>
#include "mbedtls_helper.h"
#include "mtk_huk.h"
#include "mtk_roek.h"

static uint32_t salt[] = {
	0xDBA9C093, 0xC963D274, 0xAAF635F7, 0xE67AF713,
	0x5AE9387D, 0xEFB689FB, 0x2720C4A7, 0x8C41444A
};

static uint32_t iv[] = {
	0xA531BD20, 0xEF7215F7, 0x5A6098E5, 0xB4B5885B
};

uint32_t mtk_get_roek_info_from_extern(uint8_t *key_ptr, size_t *key_len,
				       size_t size, uint8_t *cipher)
{
	uint32_t ret = 0;
	size_t len = 0;
	uint8_t enc_roek[MTK_ROEK_KEY_LEN] = {0};
	uint8_t roek[MTK_ROEK_KEY_LEN] = {0};
	uint8_t derk[MTK_ROEK_KEY_LEN] = {0};

	if (!key_ptr || !key_len || size < MTK_ROEK_KEY_LEN || !cipher)
		return MTK_ROEK_ERR_INVAL;

	*key_len = 0;
	memcpy(enc_roek, cipher, MTK_ROEK_KEY_LEN);

	ret = mtk_get_derk_info(derk, &len, sizeof(derk),
				(uint8_t *)salt, sizeof(salt),
				(uint8_t *)MTK_ROEK_KEY_NAME,
				strlen(MTK_ROEK_KEY_NAME));
	if (ret) {
		ERROR("MTK-ROEK: Derive DERK failed: %u\n", ret);
		ret = MTK_ROEK_ERR_DERIVED;
		goto mtk_get_roek_info_from_extern_err;
	}

	ret = aes_cbc_crypt(enc_roek, sizeof(enc_roek),
			    derk, len,
			    (uint8_t *)iv, sizeof(iv),
			    roek, MBEDTLS_DECRYPT);
	if (ret) {
		ERROR("MTK-ROEK: Decryption failed: %u\n", ret);
		ret = MTK_ROEK_ERR_DEC;
		goto mtk_get_roek_info_from_extern_err;
	}

	memcpy(key_ptr, roek, MTK_ROEK_KEY_LEN);
	*key_len = MTK_ROEK_KEY_LEN;

mtk_get_roek_info_from_extern_err:
	mbedtls_platform_zeroize(derk, sizeof(derk));
	mbedtls_platform_zeroize(enc_roek, sizeof(enc_roek));
	mbedtls_platform_zeroize(roek, sizeof(roek));

	return ret;
}

#ifdef MTK_ENC_ROEK
uint64_t mtk_roek_encrypt(u_register_t x1, u_register_t x2,
			  u_register_t x3, u_register_t x4,
			  void *key_ptr, size_t size)
{
	uint32_t ret = 0;
	size_t len = 0;
	uint8_t roek[MTK_ROEK_KEY_LEN] = {0};
	uint8_t enc_roek[MTK_ROEK_KEY_LEN] = {0};
	uint8_t derk[MTK_ROEK_KEY_LEN] = {0};
	uint8_t sbc_en = 1;

	ret = mtk_efuse_read(MTK_EFUSE_FIELD_SBC_EN, &sbc_en, sizeof(sbc_en));
	if (ret || sbc_en)
		return MTK_ROEK_ERR_SBC_EN;

	if (!key_ptr || size < MTK_ROEK_KEY_LEN)
		return MTK_ROEK_ERR_INVAL;

	ret = mtk_get_derk_info(derk, &len, sizeof(derk),
				(uint8_t *)salt, sizeof(salt),
				(uint8_t *)MTK_ROEK_KEY_NAME,
				strlen(MTK_ROEK_KEY_NAME));
	if (ret) {
		ERROR("MTK-ROEK: Derive DERK failed: %u\n", ret);
		ret = MTK_ROEK_ERR_DERIVED;
		goto mtk_roek_encrypt_err;
	}

	*(u_register_t *)roek = x1;
	*((u_register_t *)roek + 1) = x2;
	*((u_register_t *)roek + 2) = x3;
	*((u_register_t *)roek + 3) = x4;

	ret = aes_cbc_crypt(roek, sizeof(roek),
			    derk, len,
			    (uint8_t *)iv, sizeof(iv),
			    enc_roek, MBEDTLS_ENCRYPT);
	if (ret) {
		ERROR("MTK-ROEK: Encryption failed: %u\n", ret);
		ret = MTK_ROEK_ERR_ENC;
		goto mtk_roek_encrypt_err;
	}

	memcpy(key_ptr, enc_roek, MTK_ROEK_KEY_LEN);

mtk_roek_encrypt_err:
	mbedtls_platform_zeroize(derk, sizeof(derk));
	mbedtls_platform_zeroize(roek, sizeof(roek));
	mbedtls_platform_zeroize(enc_roek, sizeof(enc_roek));

	return ret;
}
#else
uint64_t mtk_roek_encrypt(u_register_t x1, u_register_t x2,
			  u_register_t x3, u_register_t x4,
			  void *key_ptr, size_t size)
{
	return 0;
}
#endif
