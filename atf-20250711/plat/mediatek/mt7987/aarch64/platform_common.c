/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <platform_def.h>

/* Table of regions to map using the MMU.  */
const mmap_region_t plat_mmap[] = {
#if !ENABLE_PIE
	/* for TF text, RO, RW */
	MAP_REGION_FLAT(TZRAM_BASE, TZRAM_SIZE, MT_MEMORY | MT_RW | MT_SECURE),
#endif
	MAP_REGION_FLAT(MTK_DEV_BASE, MTK_DEV_SIZE,
			MT_DEVICE | MT_RW | MT_SECURE),
	{ 0 }
};

/*******************************************************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/
#define DEFINE_CONFIGURE_MMU_EL(_el)                                           \
	void plat_configure_mmu_el##_el(                                       \
		unsigned long total_base, unsigned long total_size,            \
		unsigned long ro_start, unsigned long ro_limit,                \
		unsigned long coh_start, unsigned long coh_limit)              \
	{                                                                      \
		mmap_add_region(total_base, total_base, total_size,            \
				MT_MEMORY | MT_RW | MT_SECURE);                \
		mmap_add_region(ro_start, ro_start, ro_limit - ro_start,       \
				MT_MEMORY | MT_RO | MT_SECURE);                \
		mmap_add_region(coh_start, coh_start, coh_limit - coh_start,   \
				MT_DEVICE | MT_RW | MT_SECURE);                \
		mmap_add(plat_mmap);                                           \
		init_xlat_tables();                                            \
                                                                               \
		enable_mmu_el##_el(0);                                         \
	}

/* Define EL3 variants of the function initialising the MMU */
DEFINE_CONFIGURE_MMU_EL(3)
