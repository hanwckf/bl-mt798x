/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/delay_timer.h>
#include <platform_def.h>

void mtk_timer_init(void)
{
	write_cntfrq_el0(ARM_TIMER_CLOCK_RATE);
}
