/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>
#include <common/debug.h>

#if defined(ANTI_ROLLBACK)
#include <ar_table.h>
#elif defined(MTK_ANTI_ROLLBACK)
extern int mtk_ar_get_efuse_bl_ar_ver(uint32_t *efuse_bl_ar_ver);
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
