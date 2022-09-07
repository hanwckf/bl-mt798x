
// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <pll.h>
#include "timer.h"

void bl2pl_platform_setup(void)
{
	mtk_bl2pl_timer_init();
	mtk_pll_init(0);
}
