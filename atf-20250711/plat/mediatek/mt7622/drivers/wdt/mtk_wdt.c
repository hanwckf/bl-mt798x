/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdbool.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include <mt7622_def.h>
#include "mtk_wdt.h"

static const struct wdt_sta_reason_info {
	uint32_t bit;
	const char *reason;
} wdt_rst_reason[] = {
	{ .bit = 0,  .reason = "SPM-thermal-triggered reset" },
	{ .bit = 1,  .reason = "SPM-trigger reset" },
	{ .bit = 18, .reason = "Thermal-triggered reset" },
	{ .bit = 28, .reason = "Secure reset" },
	{ .bit = 30, .reason = "Software reset (reboot)" },
	{ .bit = 31, .reason = "Watchdog timeout" },
};

uint32_t wdt_read(uintptr_t reg)
{
	return mmio_read_32(WDT_BASE + reg);
}

void wdt_write(uintptr_t reg, uint32_t val)
{
	mmio_write_32(WDT_BASE + reg, val);
}

void wdt_rmw(uintptr_t reg, uint32_t clr, uint32_t set)
{
	uint32_t val;

	val = mmio_read_32(WDT_BASE + reg);
	val &= ~clr;
	val |= set;
	mmio_write_32(WDT_BASE + reg, val);
}

void mtk_wdt_control(bool enable)
{
	uint32_t val;

	val = wdt_read(WDT_MODE);

	if (enable) {
		if (val & WDT_MODE_ENABLE) {
			NOTICE("WDT: enabled by default\n");
			return;
		}

		NOTICE("WDT: not enabled\n");
	} else {
		if (val & WDT_MODE_ENABLE) {
			wdt_write(WDT_MODE, WDT_MODE_KEY);
			NOTICE("WDT: disabled\n");
		}
	}
}

void mtk_wdt_print_status(void)
{
	bool first_printed = false;
	uint32_t sta, i;

	sta = wdt_read(WDT_STA);
	if (!sta) {
		NOTICE("WDT: Cold boot\n");
		return;
	}

	NOTICE("WDT: [%08x] ", sta);

#if LOG_LEVEL >= LOG_LEVEL_NOTICE
	for (i = 0; i < ARRAY_SIZE(wdt_rst_reason); i++) {
		if (sta & BIT(wdt_rst_reason[i].bit)) {
			if (!first_printed) {
				first_printed = true;
				printf("%s", wdt_rst_reason[i].reason);
			} else {
				printf(", %s", wdt_rst_reason[i].reason);
			}
		}
	}

	printf("\n");
#endif
}

void mtk_wdt_soft_reset(void)
{
	wdt_rmw(WDT_MODE, WDT_MODE_DUAL_MODE | WDT_MODE_IRQ | WDT_MODE_EXTEN,
		WDT_MODE_KEY | WDT_MODE_DDR_RESERVE);
	wdt_rmw(WDT_SWRST, 0, WDT_SWRST_KEY);
}

uint32_t mtk_wdt_read_nonrst(uint32_t index)
{
	switch (index) {
	case 0:
		return wdt_read(WDT_NONRST);

	case 1:
		return wdt_read(WDT_NONRST2);

	default:
		return 0;
	}
}

void mtk_wdt_write_nonrst(uint32_t index, uint32_t val)
{
	switch (index) {
	case 0:
		wdt_write(WDT_NONRST, val);
		break;

	case 1:
		wdt_write(WDT_NONRST2, val);
		break;
	}
}
