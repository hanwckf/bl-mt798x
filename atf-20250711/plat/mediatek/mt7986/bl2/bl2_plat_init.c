/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <bl2_plat_setup.h>
#include <platform_def.h>
#include <mt7986_gpio.h>
#include <pll.h>
#include <timer.h>
#include <emi.h>
#include <mtk_wdt.h>
#ifdef I2C_SUPPORT
#include <mt_i2c.h>
#endif

/* setup clock mux/gate to default value in bl2 */
static void mtk_clock_init(void)
{

}

static void mt7986_pll_init(void)
{
#ifndef IMAGE_BL2PL
	mtk_pll_init(0);
#endif
}

static void mtk_print_cpu(void)
{
	NOTICE("CPU: MT%x (%dMHz)\n", SOC_CHIP_ID, mt_get_ckgen_ck_freq(0) * 2);
}

static void mtk_netsys_init(void)
{
	uint32_t data;

	// power-on netsys
	data = mmio_read_32(0x1001c018);
	if (data == 0x00f00000)
		mmio_write_32(0x1001c018, 0x88800000);
	else
		mmio_write_32(0x1001c018, 0x88000000);
}

static void mtk_pcie_init(void)
{
	/* power on PCIe SRAM PD */
	mmio_write_32(0x1000301c, 0x00000000);
}

static void mtk_wdt_init(void)
{
	mtk_wdt_print_status();
	mtk_wdt_control(false);
}

static void mtk_rng_init(void)
{
	mmio_write_32(0x10001070, 0x20000);
	mmio_write_32(0x10001074, 0x20000);
}

static void mtk_dcm_init(void)
{
	/* EIP_DCM_IDLE: eip_dcm_idle: Disable */
	mmio_write_32(0x10003000, 0x0);
}

static void mt7986_i2c_init(void)
{
#ifdef I2C_SUPPORT
	mtk_i2c_init();
#endif
}

void bl2_el3_plat_arch_setup(void)
{
}

const struct initcall bl2_initcalls[] = {
	INITCALL(mtk_timer_init),
	INITCALL(generic_delay_timer_init),
	INITCALL(mtk_wdt_init),
	INITCALL(mtk_pin_init),
	INITCALL(mt7986_set_default_pinmux),
	INITCALL(mt7986_pll_init),
	INITCALL(mtk_print_cpu),
	INITCALL(mt7986_i2c_init),
	INITCALL(mtk_clock_init),
	INITCALL(mtk_netsys_init),
	INITCALL(mtk_pcie_init),
	INITCALL(mtk_mem_init),
	INITCALL(mtk_rng_init),
	INITCALL(mtk_dcm_init),

	INITCALL(NULL)
};
