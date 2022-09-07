/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPUXGPT_H
#define CPUXGPT_H

#include <stdint.h>

void generic_timer_backup(void);
void plat_mt_cpuxgpt_init(void);

uint64_t tick_to_time(uint64_t ticks);
uint64_t time_to_tick(uint64_t usec);
uint64_t get_ticks(void);
uint64_t get_timer(uint64_t base);

#endif /* MT_CPUXGPT_H */
