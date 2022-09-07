/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPUXGPT_H
#define CPUXGPT_H

/* REG */
#define INDEX_CNT_L_INIT    0x008
#define INDEX_CNT_H_INIT    0x00C

void generic_timer_backup(void);
void cpuxgpt_arch_timer_init(void);
void plat_mt_cpuxgpt_init(void);

#endif /* MT_CPUXGPT_H */
