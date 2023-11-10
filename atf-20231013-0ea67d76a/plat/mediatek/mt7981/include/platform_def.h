/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLATFORM_DEF_H
#define PLATFORM_DEF_H

#include <common/interrupt_props.h>
#include <drivers/arm/gic_common.h>
#include <lib/utils_def.h>

#include "mt7981_def.h"

/*******************************************************************************
 * Platform binary types for linking
 ******************************************************************************/
#define PLATFORM_LINKER_FORMAT		"elf64-littleaarch64"
#define PLATFORM_LINKER_ARCH		aarch64

/*******************************************************************************
 * Generic platform constants
 ******************************************************************************/

/* Size of cacheable stacks */
#if defined(IMAGE_BL1)
#define PLATFORM_STACK_SIZE		0x440
#elif defined(IMAGE_BL2)
#define PLATFORM_STACK_SIZE		0x1000
#elif defined(IMAGE_BL31)
#define PLATFORM_STACK_SIZE		0x800
#elif defined(IMAGE_BL32)
#define PLATFORM_STACK_SIZE		0x440
#endif

#define FIRMWARE_WELCOME_STR		"Booting Trusted Firmware\n"

#define PLATFORM_MAX_AFFLVL		MPIDR_AFFLVL2
#define PLAT_MAX_PWR_LVL		U(2)
#define PLAT_MAX_RET_STATE		U(1)
#define PLAT_MAX_OFF_STATE		U(2)
#define PLATFORM_SYSTEM_COUNT		1
#define PLATFORM_CLUSTER_COUNT		1

#define PLATFORM_CLUSTER0_CORE_COUNT	2
#define PLATFORM_CLUSTER1_CORE_COUNT	0
#define PLATFORM_CORE_COUNT		(PLATFORM_CLUSTER1_CORE_COUNT +	\
					 PLATFORM_CLUSTER0_CORE_COUNT)
#define PLATFORM_MAX_CPUS_PER_CLUSTER	2
#define PLATFORM_NUM_AFFS		(PLATFORM_SYSTEM_COUNT +	\
					 PLATFORM_CLUSTER_COUNT +	\
					 PLATFORM_CORE_COUNT)

/*******************************************************************************
 * Platform memory map related constants
 ******************************************************************************/
/* TF txet, ro, rw, xlat table, coherent memory ... etc.
 * Size: release: 128KB, debug: 128KB
 */
#define IMAGE_LOAD_ADDR		(0x40000000)
#define TZRAM_BASE		(0x43000000)
#if DEBUG
#define TZRAM_SIZE		(0x20000)
#else
#define TZRAM_SIZE		(0x20000)
#endif

/* Reserved: 64KB */
#define TZRAM2_BASE		(TZRAM_BASE + TZRAM_SIZE)
#define TZRAM2_SIZE		(0x10000)
#define SOC_CHIP_ID		U(0x7981)

/*******************************************************************************
 * BL2 specific defines.
 ******************************************************************************/
/* BL2_BASE is defined in platform.mk */
#define BL2_LIMIT		(0x280000)

#define MAX_IO_DEVICES		U(3)
#define MAX_IO_HANDLES		U(4)
#define MAX_IO_BLOCK_DEVICES	1

/*******************************************************************************
 * BL2PL specific defines.
 ******************************************************************************/
#define BL2PL_BASE		(0x100000)
#define BL2PL_LIMIT		(0x110000)

#define L2_SRAM_BASE		(0x200000)
#define L2_SRAM_SIZE		(0x80000)

/*******************************************************************************
 * BL31 specific defines.
 ******************************************************************************/
#define BL31_BASE		(TZRAM_BASE + 0x1000)
#define BL31_LIMIT		(TZRAM_BASE + TZRAM_SIZE)

#define BL32_BASE		(TZRAM2_BASE)
#define BL32_LIMIT		(TZRAM2_BASE + TZRAM2_SIZE)
#define BL32_HEADER_SIZE	(0x1c)

#define BL33_BASE		(0x41e00000)

#define TRNG_BASE		(0x1020f000)
#define TRNG_SIZE		(0x1000)

/*******************************************************************************
 * FIP decompression specific defines.
 ******************************************************************************/
#define FIP_DECOMP_TEMP_BASE	(0x42000000)
#define FIP_DECOMP_TEMP_SIZE	(0x400000)

/*******************************************************************************
 * Platform specific page table and MMU setup constants
 ******************************************************************************/
#define PLAT_PHY_ADDR_SPACE_SIZE	(1ULL << 32)
#define PLAT_VIRT_ADDR_SPACE_SIZE	(1ULL << 32)
#define MAX_XLAT_TABLES			4
#define MAX_MMAP_REGIONS		16

/*******************************************************************************
 * Declarations and constants to access the mailboxes safely. Each mailbox is
 * aligned on the biggest cache line size in the platform. This is known only
 * to the platform as it might have a combination of integrated and external
 * caches. Such alignment ensures that two maiboxes do not sit on the same cache
 * line at any cache level. They could belong to different cpus/clusters &
 * get written while being protected by different locks causing corruption of
 * a valid mailbox address.
 ******************************************************************************/
#define CACHE_WRITEBACK_SHIFT		6
#define CACHE_WRITEBACK_GRANULE		(1 << CACHE_WRITEBACK_SHIFT)

#define PLAT_ARM_G1S_IRQ_PROPS(grp) \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_0, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_1, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_2, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_3, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_4, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_5, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_6, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE), \
	INTR_PROP_DESC(MT_IRQ_SEC_SGI_7, GIC_HIGHEST_SEC_PRIORITY, grp, \
			GIC_INTR_CFG_EDGE)

#define PLAT_ARM_G0_IRQ_PROPS(grp)

/*******************************************************************************
 * Platform specific IRQ
 ******************************************************************************/

#endif /* PLATFORM_DEF_H */
