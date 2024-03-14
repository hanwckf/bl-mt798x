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
#include <bl2_plat_setup.h>

/* IAP/REBB eFuse bit */
#define IAP_REBB_SWITCH		0x11D00A0C
#define IAP_IND			0x01

extern void mtk_mem_init_real(void);
extern int is_use_ddr4;
extern int is_use_comb;
extern int is_ddr_size_limit;
extern int is_dram_debug;
extern int is_ddr4_4bg_mode;
extern int mt7988_ddr3_freq;
extern int mt7988_ddr4_freq;
extern int mt7988_ddr_refresh_interval;
extern unsigned int mt7988_dram_size;

void mtk_mem_init(void)
{
#ifdef DRAM_USE_COMB
	is_use_comb = 1;/* DRAM_USE_COMB */
#endif
#ifdef DDR4_4BG_MODE
	NOTICE("EMI: DDR4 4BG mode\n");
	is_ddr4_4bg_mode = 1;/* 4 BG mode */
#endif

#ifdef DRAM_USE_DDR4
	is_use_ddr4 = 1;/* DRAM_USE_DDR4 */
#else 
	is_use_ddr4 = 0;/* DRAM_USE_DDR3 */
#endif

#ifdef DRAM_SIZE_LIMIT
	is_ddr_size_limit = DRAM_SIZE_LIMIT;

	if (!is_use_ddr4 && is_ddr_size_limit > 512)
		is_ddr_size_limit = 512;
#endif /* DRAM_SIZE_LIMIT */

#ifdef DRAM_DEBUG_LOG
	is_dram_debug = 1;
#endif /* DRAM_DEBUG_LOG */

#ifdef DDR3_FREQ_2133
	mt7988_ddr3_freq = 2133;
#endif /* DDR3_FREQ_2133 */
#ifdef DDR3_FREQ_1866
	mt7988_ddr3_freq = 1866;
#endif /* DDR3_FREQ_1866 */

#ifdef DDR_REFRESH_INTERVAL_780
	mt7988_ddr_refresh_interval = 0xca;
#endif
#ifdef DDR_REFRESH_INTERVAL_390
	mt7988_ddr_refresh_interval = 0x65;
#endif
#ifdef DDR_REFRESH_INTERVAL_292
	mt7988_ddr_refresh_interval = 0x4c;
#endif
#ifdef DDR_REFRESH_INTERVAL_195
	mt7988_ddr_refresh_interval = 0x33;
#endif

#ifdef DDR4_FREQ_3200
	mt7988_ddr4_freq = 3200;
#endif /* DDR4_FREQ_3200 */
#ifdef DDR4_FREQ_2666
	mt7988_ddr4_freq = 2666;
#endif /* DDR4_FREQ_2666 */

	if (!is_use_comb)
		NOTICE("EMI: Using DDR%u settings\n", is_use_ddr4 ? 4 : 3);
	else
		NOTICE("EMI: Using DDR unknown settings\n");

	mtk_mem_init_real();

	mtk_bl2_set_dram_size((uint64_t)mt7988_dram_size << 20);
}

void mtk_mem_dbg_print(const char *fmt, ...)
{
	va_list args;

	if (!is_dram_debug)
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
