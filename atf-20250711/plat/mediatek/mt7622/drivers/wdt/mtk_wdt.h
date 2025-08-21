/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MTK_WDT_H
#define MTK_WDT_H

#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>

#define WDT_MODE			0x00
#define WDT_STA				0x0c
#define WDT_SWRST			0x14
#define WDT_NONRST			0x20
#define WDT_NONRST2			0x24
#define WDT_DEBUG_CTL			0x40

/* WDT_MODE */
#define WDT_MODE_KEY			0x22000000
#define WDT_MODE_DDR_RESERVE		BIT(7)
#define WDT_MODE_DUAL_MODE		BIT(6)
#define WDT_MODE_IRQ			BIT(3)
#define WDT_MODE_EXTEN			BIT(2)
#define WDT_MODE_ENABLE			BIT(0)

/* WDT_STA */
#define WDT_REASON_HW_RST		BIT(31)
#define WDT_REASON_SW_RST		BIT(30)
#define WDT_REASON_SECURE_RST		BIT(28)
#define WDT_REASON_THERMAL_RST		BIT(18)
#define WDT_REASON_SPM_RST		BIT(1)
#define WDT_REASON_SPM_THERMAL_RST	BIT(0)

/* WDT_SWRST */
#define WDT_SWRST_KEY			0x1209

/* WDT_DEBUG_CTL */
#define WDT_DEBUG_CTL_KEY		0x59000000
#define WDT_DDR_SREF_STA		BIT(17)
#define WDT_DDR_RESERVE_STA		BIT(16)
#define WDT_RG_CONF_ISO			BIT(10)
#define WDT_RG_DRAMC_ISO		BIT(9)
#define WDT_RG_DRAMC_SREF		BIT(8)

uint32_t wdt_read(uintptr_t reg);
void wdt_write(uintptr_t reg, uint32_t val);
void wdt_rmw(uintptr_t reg, uint32_t clr, uint32_t set);

void mtk_wdt_control(bool enable);
void mtk_wdt_print_status(void);
void mtk_wdt_soft_reset(void);

uint32_t mtk_wdt_read_nonrst(uint32_t index);
void mtk_wdt_write_nonrst(uint32_t index, uint32_t val);

#endif /* MTK_WDT_H */
