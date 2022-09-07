/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>
#include <common/debug.h>
#include <ar_table.h>

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
#ifdef ANTI_ROLLBACK
	int tfw_ar_ver = mtk_antirollback_get_tfw_ar_ver();

	if (tfw_ar_ver < 0) {
		ERROR("[%s] get TFW_AR_VER fail\n", __func__);
		return 1;
	}

	*nv_ctr = tfw_ar_ver;
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
