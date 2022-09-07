/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MTK_MEM_POOL_H
#define MTK_MEM_POOL_H

#include <stdint.h>
#include <stddef.h>

void mtk_mem_pool_set_base(uintptr_t base);
void *mtk_mem_pool_alloc(size_t size);

#endif /* MTK_MEM_POOL_H */
