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
extern int mt7986_use_ddr4;
extern int mt7986_ddr4_freq;
extern int mt7986_ddr_size_limit;
extern int mt7986_dram_debug;

void mtk_mem_init(void)
{
#ifdef DRAM_USE_DDR4
#if 0	/* Enable this after eFuse bit is ready */
	/* Only IAP supports DDR4 */
	if (mmio_read_32(IAP_REBB_SWITCH) & IAP_IND)
#endif
		mt7986_use_ddr4 = 1;
#endif /* DRAM_USE_DDR4 */

#ifdef DRAM_SIZE_LIMIT
	mt7986_ddr_size_limit = DRAM_SIZE_LIMIT;

	if (!mt7986_use_ddr4 && mt7986_ddr_size_limit > 512)
		mt7986_ddr_size_limit = 512;
#endif /* DRAM_SIZE_LIMIT */

#ifdef DRAM_DEBUG_LOG
	mt7986_dram_debug = 1;
#endif /* DRAM_DEBUG_LOG */

#ifdef DDR4_FREQ_3200
	mt7986_ddr4_freq = 3200;
#endif /* DDR4_FREQ_3200 */
#ifdef DDR4_FREQ_2666
	mt7986_ddr4_freq = 2666;
#endif /* DDR4_FREQ_2400 */

	NOTICE("EMI: Using DDR%u settings\n", mt7986_use_ddr4 ? 4 : 3);

	mtk_mem_init_real();
}

void mtk_mem_dbg_print(const char *fmt, ...)
{
	va_list args;

	if (!mt7986_dram_debug)
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
