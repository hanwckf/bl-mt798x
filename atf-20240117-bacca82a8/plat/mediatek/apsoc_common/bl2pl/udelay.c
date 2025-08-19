/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/delay_timer.h>

void udelay(uint32_t usec)
{
	uint64_t tmo = timeout_init_us(usec);

	while (!timeout_elapsed(tmo))
		;
}
