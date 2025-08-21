/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <lib/mmio.h>
#include <drivers/generic_delay_timer.h>
#include <bl2_plat_setup.h>
#include <platform_def.h>
#include <emi.h>
#include <mtk_wdt.h>
#include <pinctrl.h>
#include <pll.h>
#include <pmic.h>
#include <pmic_wrap_init.h>
#include <cpuxgpt.h>

static void mtk_print_cpu(void)
{
	NOTICE("CPU: MT%x\n", SOC_CHIP_ID);
}

static void mtk_wdt_init(void)
{
	mtk_wdt_control(false);
}

void bl2_el3_plat_arch_setup(void)
{
}

const struct initcall bl2_initcalls[] = {
	INITCALL(plat_mt_cpuxgpt_init),
	INITCALL(generic_delay_timer_init),
	INITCALL(mtk_wdt_print_status),
	INITCALL(mtk_print_cpu),
	INITCALL(mtk_pin_init),
#ifndef IMAGE_BL2PL
	INITCALL(mtk_pll_init),
#endif
	INITCALL(mtk_pwrap_init),
	INITCALL(mtk_pmic_init),
	INITCALL(mtk_mem_init),
	INITCALL(mtk_wdt_init),

	INITCALL(NULL)
};
