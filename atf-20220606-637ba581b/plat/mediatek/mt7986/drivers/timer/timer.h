/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef __MTK_TIMER_H__
#define __MTK_TIMER_H__

#include <stdint.h>

void mtk_timer_init(void);
void basic_timer_test(void);
uint64_t get_ticks(void);
uint64_t get_timer(uint64_t base);
uint64_t tick_to_time(uint64_t ticks);
uint64_t time_to_tick(uint64_t usec);
void __udelay(uint64_t usec);
void __mdelay(uint64_t msec);

#endif
