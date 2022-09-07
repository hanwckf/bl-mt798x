
// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <pll.h>
#include <cpuxgpt.h>

void bl2pl_platform_setup(void)
{
	plat_mt_cpuxgpt_init();
	mtk_pll_init();
}
