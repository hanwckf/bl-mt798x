/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef MTK_WDT_H
#define MTK_WDT_H

#include <stdint.h>
#include <stdbool.h>

void mtk_wdt_control(bool enable);
void mtk_wdt_print_status(void);
void mtk_wdt_soft_reset(void);

uint32_t mtk_wdt_read_nonrst(uint32_t index);
void mtk_wdt_write_nonrst(uint32_t index, uint32_t val);

#endif /* MTK_WDT_H */
