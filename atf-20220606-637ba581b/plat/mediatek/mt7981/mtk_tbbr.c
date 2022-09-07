/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>
#include <common/debug.h>

extern int mtk_ar_get_efuse_bl_ar_ver(uint32_t *efuse_bl_ar_ver);
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
#ifdef MTK_ANTI_ROLLBACK
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

int plat_get_mbedtls_heap(void **heap_addr, size_t *heap_size)
{
	return get_mbedtls_heap_helper(heap_addr, heap_size);
}
