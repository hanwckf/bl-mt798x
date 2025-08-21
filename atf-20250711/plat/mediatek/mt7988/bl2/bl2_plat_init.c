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
#include <mt7988_gpio.h>
#include <cpuxgpt.h>
#include <pll.h>
#include <emi.h>
#include <mtk_wdt.h>
#ifdef EIP197_SUPPORT
#include <eip197_init.h>
#endif
#ifdef I2C_SUPPORT
#include <mt_i2c.h>
#include "mt6682a.h"
#endif

#define CHN_EMI_TESTB		0x10236048
#define EMI_TESTB		0x102190e8
#define GDU_REG_18		0x11c40048
#define GBE_TOP_REG		0x11d1020c
#define   I2P5G_MDIO		BIT(22)
#define CL22_REG0_BY_PBUS	0x0f800000
#define   CL22_PHY_POWER_DOWN	BIT(11)
#define PGD_REG_9		0x11d40024
#define PGD_REG_33		0x11d40084
#define WDT_REQ_IRQ_EN		(WDT_BASE + 0x34)
#define WDT_REQ_IRQ_EN_KEY	0x44000000

#define HANG_FREE_PROT_INFRA_AO	0x1000310c

static void mtk_wdt_init(void)
{
	mtk_wdt_print_status();
	mtk_wdt_control(false);
}

static void mt7988_pll_init(void)
{
#ifndef IMAGE_BL2PL
	mtk_pll_init(0);
#endif

	plat_mt_cpuxgpt_pll_update_sync();
}

static void mtk_print_cpu(void)
{
#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
	VERBOSE("CPU: MT%x (%dMHz)\n", SOC_CHIP_ID, mtk_get_cpu_freq());
#else
	NOTICE("CPU: MT%x\n", SOC_CHIP_ID);
#endif
}

static void mtk_disable_GDU(void)
{
	INFO("PGD: disable GDU\n");
	mmio_write_32(GDU_REG_18, 0x1); /* mask analog output */
}

static void mtk_disable_PGD(void)
{
	INFO("PGD: disable PGD\n");
	mmio_write_32(PGD_REG_33, 0xFFF); /* mask analog output */
	mmio_write_32(PGD_REG_9, 0x40); /* DA_PWRGD_ENB analog power down */
}

static void mtk_wed_init(void)
{
	INFO("WED: setup initial settings\n");
	/* EMI_TESTB: BYTE32_WRAP_EN */
	mmio_setbits_32(EMI_TESTB, 0x20);
	/* EMI_TESTB: CHN_EMI_TESTB */
	mmio_setbits_32(CHN_EMI_TESTB, 0x20);
}

static void mtk_infra_ao_init(void)
{
	mmio_write_32(HANG_FREE_PROT_INFRA_AO, 0x0);
}

static void mtk_i2p5g_phy_init(void)
{
	/* For internal 2.5Gphy,
	 * set bit 22 to use internal MDIO, and
	 * clear bit 22 to use external MDIO.
	 */
	mmio_setbits_32(GBE_TOP_REG, I2P5G_MDIO);
	/* Internal 2.5Gphy power on sequence */
	eth_2p5g_phy_mtcmos_ctrl(true);
	/* Do CL22 phy power down via pbus. */
	mmio_setbits_32(CL22_REG0_BY_PBUS, CL22_PHY_POWER_DOWN);
}

static void mt7988_i2c_init(void)
{
#ifdef I2C_SUPPORT
	mtk_i2c_init();
	mt6682a_init();
#ifdef CPU_USE_FULL_SPEED
	mt6682a_set_voltage(REGULATOR_BUCK3, 900000, 900000);
#endif
#endif
}

static void mt7988_enable_lvts_hw_reset(void)
{
	NOTICE("LVTS: Enable thermal HW reset\n");
	mmio_clrsetbits_32(WDT_REQ_IRQ_EN, BIT(16), WDT_REQ_IRQ_EN_KEY);
}

static void mtk_pcie_init(void)
{
	uint32_t efuse, intr, cktx_pt0, cktx_pt1, cktx_pt2, cktx_pt3;

	/* PCIe SRAM PD  0:power on, 1:power off
	 * 0x10003030 bit[11:0]  port0 SRAM PD
	 * 0x10003030 bit[23:12] port1 SRAM PD
	 * 0x10003034 bit[11:0]  port2 SRAM PD
	 * 0x10003034 bit[23:12] port3 SRAM PD*/
	mmio_write_32(0x10003030, 0x00000000);
	mmio_write_32(0x10003034, 0x00000000);

	/* Switch PCIe MSI group1 to AP */
	mmio_write_32(0x10209000, 0x00000007);

	/* Adjust EQ preset P10 coefficient */
	mmio_write_32(0x11e48038, 0x000f2100);
	mmio_write_32(0x11e48138, 0x000f2100);
	mmio_write_32(0x11e58038, 0x000f2100);
	mmio_write_32(0x11e58138, 0x000f2100);
	mmio_write_32(0x11e68038, 0x000f2100);
	mmio_write_32(0x11e78038, 0x000f2100);

	/* CKM efuse load */
	efuse = mmio_read_32(0x11f508c4);
	intr = (efuse & GENMASK(25, 20)) >> 4;
	cktx_pt0 = (efuse & GENMASK(19, 15)) >> 15;
	cktx_pt1 = (efuse & GENMASK(14, 10)) >> 10;
	cktx_pt2 = (efuse & GENMASK(9, 5)) << 11;
	cktx_pt3 = efuse & GENMASK(4, 0);
	/* BIAS_INTR_CTRL:  EFUSE[25:20] to 0x11f10000[21:16] */
	mmio_clrsetbits_32(0x11f10000, GENMASK(21, 16), intr);
	/* PT0_CKTX_IMPSEL: EFUSE[19:15] to 0x11f10004[4:0] */
	mmio_clrsetbits_32(0x11f10004, GENMASK(4, 0), cktx_pt0);
	/* PT1_CKTX_IMPSEL: EFUSE[14:10] to 0x11f10018[4:0] */
	/* PT2_CKTX_IMPSEL: EFUSE[9:5]   to 0x11f10018[20:16] */
	mmio_clrsetbits_32(0x11f10018, GENMASK(4, 0) | GENMASK(20, 16),
			   cktx_pt1 | cktx_pt2);
	/* PT3_CKTX_IMPSEL: EFUSE[4:0]   to 0x11f1001c[4:0] */
	mmio_clrsetbits_32(0x11f1001c, GENMASK(4, 0), cktx_pt3);

	/* Disable CKM internal 50ohm pull down */
	mmio_clrbits_32(0x11f10004, BIT(5));
	mmio_clrbits_32(0x11f10018, BIT(5) | BIT(21));
	mmio_clrbits_32(0x11f1001c, BIT(5));
}

static void mtk_eip197_init(void)
{
#ifdef EIP197_SUPPORT
	NOTICE("EIP197: setup initial setting\n");
	eip197_init();
#endif
}

void bl2_el3_plat_arch_setup(void)
{
	plat_mt_cpuxgpt_init();
	generic_delay_timer_init();
}

const struct initcall bl2_initcalls[] = {
	INITCALL(mtk_wdt_init),
	INITCALL(mtk_disable_PGD),
	INITCALL(mtk_disable_GDU),
	INITCALL(mtk_pin_init),
	INITCALL(mt7988_set_default_pinmux),
	INITCALL(mt7988_i2c_init),
	INITCALL(mt7988_pll_init),
	INITCALL(mtk_print_cpu),
	INITCALL(mtk_infra_ao_init),
	INITCALL(mtk_pcie_init),
	INITCALL(mtk_eip197_init),
	INITCALL(mtk_mem_init),
	INITCALL(mtk_wed_init),
	INITCALL(mtk_i2p5g_phy_init),
	INITCALL(mt7988_enable_lvts_hw_reset),

	INITCALL(NULL)
};
