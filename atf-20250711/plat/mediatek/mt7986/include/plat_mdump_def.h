/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PLAT_MDUMP_DEF_H_
#define _PLAT_MDUMP_DEF_H_

#include <platform_def.h>
#include "plat_eth_def.h"
#include "memdump.h"

#define DRAM_START		0x40000000ULL
#define DRAM_END		0xC0000000ULL

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
			/* We have to borrow 32KB for DMA. Please make sure reserved memory is large enough in dts. */
			.addr = TZRAM_END + 0x8000,
			.end = DRAM_END,
		},
		.need_map = true,
	}
};

static inline size_t plat_get_mdump_ranges(const struct mdump_range **retranges)
{
	(*retranges) = mdump_ranges;

	return ARRAY_SIZE(mdump_ranges);
}

static inline uint32_t plat_get_rnd(uint32_t *val)
{
	*val = 0;
	return 0;
}

#endif /* _PLAT_MDUMP_DEF_H_ */
