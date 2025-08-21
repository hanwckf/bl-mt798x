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
#include <mt7981_gpio.h>
#include <pll.h>
#include <timer.h>
#include <emi.h>
#include <mtk_wdt.h>

static void mt7981_pll_init(void)
{
#ifndef IMAGE_BL2PL
	mtk_pll_init(0);
#endif
}

static void mtk_print_cpu(void)
{
	NOTICE("CPU: MT%x (%uMHz)\n", SOC_CHIP_ID, mtk_get_cpu_freq());
}

static void mtk_wdt_init(void)
{
	mtk_wdt_print_status();
	mtk_wdt_control(false);
}

static void mtk_sgmii_init(void)
{
	unsigned int val;

	val = mmio_read_32(SGMII0_CAL_DATA);
	if (!val) {
		INFO("SGMII0_CAL_DATA: Default\n");
		mmio_write_32(SGMII_PHY0_FTCTL, 0x24101007);
	}
	val = mmio_read_32(SGMII1_CAL_DATA);
	if (!val) {
		INFO("SGMII1_CAL_DATA: Default\n");
		mmio_write_32(SGMII_PHY1_FTCTL, 0x24101007);
	}
}

static void mtk_pcie_init(void)
{
	/* power on PCIe SRAM PD */
	mmio_write_32(0x1000301c, 0x00000000);
}

void bl2_el3_plat_arch_setup(void)
{
}

const struct initcall bl2_initcalls[] = {
	INITCALL(mtk_timer_init),
	INITCALL(generic_delay_timer_init),
	INITCALL(mtk_wdt_init),
	INITCALL(mtk_pin_init),
	INITCALL(mt7981_set_default_pinmux),
	INITCALL(mt7981_pll_init),
	INITCALL(mtk_sgmii_init),
	INITCALL(mtk_pcie_init),
	INITCALL(mtk_mem_init),
	INITCALL(mtk_print_cpu),

	INITCALL(NULL)
};
