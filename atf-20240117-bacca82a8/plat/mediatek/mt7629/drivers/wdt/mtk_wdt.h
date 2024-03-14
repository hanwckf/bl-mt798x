/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MTK_WDT_H
#define MTK_WDT_H

#include <stdint.h>
#include <stdbool.h>

void mtk_wdt_control(bool enable);
void mtk_wdt_print_status(void);
void mtk_wdt_soft_reset(void);

#endif /* MTK_WDT_H */
