/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <platform_def.h>

void mtk_timer_init(void)
{
	write_cntfrq_el0(ARCH_TIMER_CLK);
}
