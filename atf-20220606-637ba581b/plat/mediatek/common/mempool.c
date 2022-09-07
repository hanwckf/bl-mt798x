/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stddef.h>

static uintptr_t mtk_mem_pool_base = MTK_MEM_POOL_BASE;

void mtk_mem_pool_set_base(uintptr_t base)
{
	mtk_mem_pool_base = base;
}

void *mtk_mem_pool_alloc(size_t size)
{
	uintptr_t base;

	mtk_mem_pool_base += 0x7f;
	mtk_mem_pool_base &= ~0x7f;

	base = mtk_mem_pool_base;
	mtk_mem_pool_base += size;

	return (void *)base;
}
