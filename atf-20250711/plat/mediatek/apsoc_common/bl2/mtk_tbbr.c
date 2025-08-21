// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2018, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <plat/common/platform.h>
#include <common/debug.h>

#if defined(ANTI_ROLLBACK)
#include <ar_table.h>
#elif defined(MTK_ANTI_ROLLBACK)
extern int mtk_ar_get_efuse_bl_ar_ver(uint32_t *efuse_bl_ar_ver);
#endif

#ifdef MTK_FIP_ENC
#include <salt.h>
#include <tools_share/firmware_encrypted.h>
#include <mtk_roe.h>
#include <key_info.h>
#endif

extern char mtk_rotpk_hash[], mtk_rotpk_hash_end[];

int plat_get_rotpk_info(void *cookie, void **key_ptr, unsigned int *key_len,
			unsigned int *flags)
{
	*key_ptr = mtk_rotpk_hash;
	*key_len = mtk_rotpk_hash_end - mtk_rotpk_hash;
	*flags = ROTPK_IS_HASH;

	return 0;
}

int plat_get_nv_ctr(void *cookie, unsigned int *nv_ctr)
{
#if defined(ANTI_ROLLBACK)
	int tfw_ar_ver = mtk_antirollback_get_tfw_ar_ver();

	if (tfw_ar_ver < 0) {
		ERROR("[%s] get TFW_AR_VER fail\n", __func__);
		return 1;
	}

	*nv_ctr = tfw_ar_ver;
#elif defined(MTK_ANTI_ROLLBACK)
	int ret;
	uint32_t efuse_bl_ar_ver;

	ret = mtk_ar_get_efuse_bl_ar_ver(&efuse_bl_ar_ver);
	if (ret) {
		ERROR("[%s] get EFUSE_BL_AR_VER fail\n", __func__);
		return ret;
	}

	*nv_ctr = efuse_bl_ar_ver;
#else
	*nv_ctr = 0;
#endif
	return 0;
}

int plat_set_nv_ctr(void *cookie, unsigned int nv_ctr)
{
	return 0;
}

#ifdef MTK_FIP_ENC
int plat_get_enc_key_info(enum fw_enc_status_t fw_enc_status, uint8_t *key,
			  size_t *key_len, unsigned int *flags,
			  const uint8_t *img_id, size_t img_id_len)
{
	uint8_t salt[SALT_SIZE] = FIP_KEY_SALT;
	int ret = 0;

	ret = derive_from_roe_key(salt, SALT_SIZE, key, FIP_KEY_SIZE);
	if (ret) {
		ERROR("%s: derive key failed: %d\n", __func__, ret);
		return ret;
	}
	*flags = 0;
	*key_len = FIP_KEY_SIZE;

	return ret;
}
#endif
