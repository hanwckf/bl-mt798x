/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PLAT_MDUMP_DEF_H_
#define _PLAT_MDUMP_DEF_H_

#include <common/bl_common.h>
#include "plat_eth_def.h"
#include <rng.h>
#include "memdump.h"

#define DRAM_START		0x40000000ULL
#define DRAM_END		0x140000000ULL

#if ENABLE_PIE
static struct mdump_range mdump_ranges[] = {
	{
		.r = {
			.addr = GIC_BASE,
			.end = GIC_BASE + 0x100000,
		},
		.is_device = true,
	},
#ifdef NEED_BL32
	{
		.r = {
			.addr = DRAM_START,
			.end = BL32_TZRAM_BASE,
		},
		.need_map = true,
	},
	{
		.r = {
			.addr = BL32_TZRAM_BASE + BL32_TZRAM_SIZE,
			.end = DRAM_END,
		},
		.need_map = true,
	}
#else
	{
		.r = {
			.addr = DRAM_START,
			.end = DRAM_END,
		},
		.need_map = true,
	}
#endif
};
#else
#ifdef NEED_BL32
#define TZRAM_END		(TZRAM_BASE + TZRAM_SIZE + TZRAM2_SIZE)
#else
#define TZRAM_END		(TZRAM_BASE + TZRAM_SIZE)
#endif

static const struct mdump_range mdump_ranges[] = {
	{
		.r = {
			.addr = GIC_BASE,
			.end = GIC_BASE + 0x100000,
		},
		.is_device = true,
	},
	{
		.r = {
			.addr = DRAM_START,
			.end = TZRAM_BASE,
		},
		.need_map = true,
	},
	{
		.r = {
			.addr = TZRAM_END,
			.end = DRAM_END,
		},
		.need_map = true,
	}
};
#endif

static inline size_t plat_get_mdump_ranges(const struct mdump_range **retranges)
{
#if ENABLE_PIE
#ifdef NEED_BL32
	mdump_ranges[2].r.end = BL31_START - BL31_LOAD_OFFSET;
#else
	mdump_ranges[1].r.end = BL31_START - BL31_LOAD_OFFSET;
#endif
#endif

	(*retranges) = mdump_ranges;

	return ARRAY_SIZE(mdump_ranges);
}

#endif /* _PLAT_MDUMP_DEF_H_ */
