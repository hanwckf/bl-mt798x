
// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <cpuxgpt.h>

void udelay(uint32_t usec)
{
	uint64_t st, delta;

	st = get_timer(0);
	delta = usec + st + 1;

	while (get_timer(0) < delta)
		;
}
