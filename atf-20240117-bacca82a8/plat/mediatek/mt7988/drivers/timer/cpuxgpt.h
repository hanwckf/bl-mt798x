/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef CPUXGPT_H
#define CPUXGPT_H

void generic_timer_backup(void);
void plat_mt_cpuxgpt_init(void);
void plat_mt_cpuxgpt_pll_update_sync(void);

#endif /* MT_CPUXGPT_H */
