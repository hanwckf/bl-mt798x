/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7988_MCUCFG_H
#define MT7988_MCUCFG_H

#include <platform_def.h>
#include <stdint.h>
#include <mt7988_def.h>

struct mt7988_mcucfg_regs {
	uint32_t adb_cfg; /* 100E2000 */
	uint32_t ptp3_udi; /* 100E2004 */
	uint32_t pwr_rst_ctl; /* 100E2008 */
	uint32_t cache_ctl; /* 100E200C */
	uint32_t armpll_div_clkoff; /* 100E2010 */
	uint32_t esr_case; /* 100E2014 */
	uint32_t esr_mask; /* 100E2018 */
	uint32_t esr_trig; /* 100E201C */
	uint32_t reserved_0[120];
	uint32_t delsel0; /* 100E2200 */
	uint32_t reserved_1; /* 100E2204 */
	uint32_t cpucfg; /* 100E2208 */
	uint32_t miscdbg; /* 100E220C */
	uint32_t dbgromaddr; /* 100E2210 */
	uint32_t dbgselfaddr; /* 100E2214 */
	uint32_t acecfg; /* 100E2218 */
	uint32_t reserved_2;
	uint32_t fuse0; /* 100E2220 */
	uint32_t fuse1; /* 100E2224 */
	uint32_t reserved_3[10];
	uint32_t rgu_setting; /* 100E2250 */
	uint32_t rst_ctl; /* 100E2254 */
	uint32_t reserved_4[2];
	uint32_t clkenm_div; /* 100E2260 */
	uint32_t int_mask; /* 100E2264 */
	uint32_t clk_en; /* 100E2268 */
	uint32_t reserved_5;
	uint32_t armpll_div; /* 100E2270 */
	uint32_t sync_dcm; /* 100E2274 */
	uint32_t reserved_6[2];
	uint32_t cpu_cfg_2; /* 100E2280 */
	uint32_t reserved_7[3];
	uint32_t rvaddr0_l; /* 100E2290 */
	uint32_t rvaddr0_h; /* 100E2294 */
	uint32_t rvaddr1_l; /* 100E2298 */
	uint32_t rvaddr1_h; /* 100E229C */
	uint32_t ptp3_cputop_spmc0; /* 100E22A0 */
	uint32_t ptp3_cputop_spmc1; /* 100E22A4 */
	uint32_t reserved_8[4];
	uint32_t sramdreq; /* 100E22B8 */
	uint32_t cqq; /* 100E22BC */
	uint32_t rvaddr2_l; /* 100E22C0 */
	uint32_t rvaddr2_h; /* 100E22C4 */
	uint32_t rvaddr3_l; /* 100E22C8 */
	uint32_t rvaddr3_h; /* 100E22CC */
};

static struct mt7988_mcucfg_regs *const mt7988_mcucfg =
	(void *)MCUCFG_BASE + 0x2000;
void mt7988_mcucfg_selftest(void);

/* cpu boot mode */
#define MP0_CPUCFG_64BIT_SHIFT 16
#define MP0_CPUCFG_64BIT       (U(0xf) << MP0_CPUCFG_64BIT_SHIFT)

#endif /* MT7988_MCUCFG_H */
