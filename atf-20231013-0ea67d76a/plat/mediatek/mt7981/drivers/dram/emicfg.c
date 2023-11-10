/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <stdarg.h>
#include <stdio.h>

/* IAP/REBB eFuse bit */
#define IAP_REBB_SWITCH		0x11D00A0C
#define IAP_IND			0x01

extern void mtk_mem_init_real(void);
extern int mt7981_use_ddr4;
extern int mt7981_ddr_size_limit;
extern int mt7981_dram_debug;
extern int mt7981_bga_pkg;
extern int mt7981_ddr3_freq;

void mtk_mem_init(void)
{
#ifdef DRAM_USE_DDR4
		mt7981_use_ddr4 = 1;
#endif /* DRAM_USE_DDR4 */

#ifdef DRAM_SIZE_LIMIT
	mt7981_ddr_size_limit = DRAM_SIZE_LIMIT;

	if (!mt7981_use_ddr4 && mt7981_ddr_size_limit > 512)
		mt7981_ddr_size_limit = 512;
#endif /* DRAM_SIZE_LIMIT */

#ifdef DRAM_DEBUG_LOG
	mt7981_dram_debug = 1;
#endif /* DRAM_DEBUG_LOG */

#if defined(BOARD_BGA)
	mt7981_bga_pkg = 1;
#elif defined(BOARD_QFN)
	mt7981_bga_pkg = 0;
#endif /* BOARD_BGA */

#ifdef DDR3_FREQ_2133
	mt7981_ddr3_freq = 2133;
#endif /* DDR3_FREQ_2133 */
#ifdef DDR3_FREQ_1866
	mt7981_ddr3_freq = 1866;
#endif /* DDR3_FREQ_1866 */

	NOTICE("EMI: Using DDR%u settings\n", mt7981_use_ddr4 ? 4 : 3);

	mtk_mem_init_real();
}

void mtk_mem_dbg_print(const char *fmt, ...)
{
	va_list args;

	if (!mt7981_dram_debug)
		return;

	va_start(args, fmt);
	(void)vprintf(fmt, args);
	va_end(args);
}

void mtk_mem_err_print(const char *fmt, ...)
{
	const char *prefix_str;
	va_list args;

	prefix_str = plat_log_get_prefix(LOG_LEVEL_ERROR);

	while (*prefix_str != '\0') {
		(void)putchar(*prefix_str);
		prefix_str++;
	}

	va_start(args, fmt);
	(void)vprintf(fmt, args);
	va_end(args);
}
