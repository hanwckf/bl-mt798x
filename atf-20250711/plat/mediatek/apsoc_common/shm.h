
/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef _SHM_H
#define _SHM_H

#include <lib/xlat_tables/xlat_tables_v2.h>
#include <mbedtls/memory_buffer_alloc.h>

#define FREE_SHARED_MEMORY_ERR		0x0F00
#define ALLOC_SHARED_MEMORY_ERR		0x0F01

#define SHM_OFFSET(paddr) (paddr - (paddr & TABLE_ADDR_MASK))
#define ALIGN_SHM_SIZE(paddr, size) \
	(((paddr & PAGE_SIZE_MASK) + (size + PAGE_SIZE - 1)) & ~(size_t)(PAGE_SIZE - 1))

int set_shared_memory(uintptr_t paddr, size_t shm_size, uintptr_t *shm_vaddr, uint32_t flag);
int free_shared_memory(uintptr_t shm_vaddr, size_t shm_size);

#endif
