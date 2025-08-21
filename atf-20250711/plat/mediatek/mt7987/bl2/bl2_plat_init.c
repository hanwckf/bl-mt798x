// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <lib/mmio.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <bl2_plat_setup.h>
#include <platform_def.h>
#include <mt7987_gpio.h>
#include <cpuxgpt.h>
#include <pll.h>
#include <emi.h>
#include <mtk_wdt.h>
#ifdef I2C_SUPPORT
#include <mt_i2c.h>
#include <at24c.h>
#endif

#define CHN_EMI_TESTB		0x10236048
#define EMI_TESTB		0x102190e8
#define GBE_TOP_REG		0x1002120c
#define   I2P5G_MDIO		BIT(22)
#define CL22_REG0_BY_PBUS	0x0f800000
#define   CL22_PHY_POWER_DOWN	BIT(11)
#define PGD_REG_9		0x11d40024
#define PGD_REG_33		0x11d40084
#define WDT_REQ_MODE		(WDT_BASE + 0x30)
#define WDT_REQ_MODE_KEY	0x33000000
#define WDT_REQ_IRQ_EN		(WDT_BASE + 0x34)
#define WDT_REQ_IRQ_EN_KEY	0x44000000
#define HANG_FREE_PROT_INFRA_AO	0x1000310c

static void mtk_wdt_init(void)
{
	mtk_wdt_print_status();
	mtk_wdt_control(false);
}

static void mt7987_pll_init(void)
{
#ifndef IMAGE_BL2PL
	mtk_pll_init(0);
#endif

	plat_mt_cpuxgpt_pll_update_sync();
}

static void mtk_print_cpu(void)
{
	const char *mt7987_name = "MT7987";
	const char *mt7987a_name = "MT7987A";
	const char *mt7987b_name = "MT7987B";
	const char *mt7987ai_name = "MT7987AI";
	const char *chip_name;
	uint32_t chip_id;

	chip_id = mmio_read_32(0x11d3090c) & GENMASK(2, 0);

	switch (chip_id) {
		case 0x0:
			chip_name = mt7987a_name;
			break;
		case 0x1:
			chip_name = mt7987b_name;
			break;
		case 0x4:
			chip_name = mt7987ai_name;
			break;
		default:
			chip_name = mt7987_name;
			break;
	}

#if LOG_LEVEL >= LOG_LEVEL_VERBOSE
	VERBOSE("CPU: %s (%dMHz)\n", chip_name, mtk_get_cpu_freq());
#else
	NOTICE("CPU: %s\n", chip_name);
#endif
}

static void mtk_disable_PGD(void)
{
	INFO("PGD: disable PGD\n");
	mmio_write_32(PGD_REG_33, 0xFFFFFF);
	mmio_write_32(PGD_REG_9, 0x40);
	mmio_clrsetbits_32(WDT_REQ_MODE, BIT(20), WDT_REQ_MODE_KEY);
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

static void mt7987_i2c_init(void)
{
#ifdef I2C_SUPPORT
	/* Set pinmux to GPIO mode for i2c_gpio */
	mt_set_pinmux_mode(I2C_GPIO_SCL_PIN, GPIO_MODE_00);
	mt_set_pinmux_mode(I2C_GPIO_SDA_PIN, GPIO_MODE_00);

	mtk_i2c_init();

	/* Call the I2C API located under
         * plat/mediatek/apsoc_common/drivers/i2c here
         */

	/* ... */
#endif
}

static void mt7987_enable_lvts_hw_reset(void)
{
	NOTICE("LVTS: Enable thermal HW reset\n");
	mmio_clrsetbits_32(WDT_REQ_IRQ_EN, BIT(16), WDT_REQ_IRQ_EN_KEY);
}

static void mtk_pcie_init(void)
{
	uint32_t efuse, intr, cktx_pt0, cktx_pt1;

	/* PCIe SRAM PD  0:power on, 1:power off
	 * 0x10003030 bit[11:0]  port0 SRAM PD
	 * 0x10003030 bit[23:12] port1 SRAM PD
	 */
	mmio_write_32(0x10003030, 0x00000000);

	/* Switch PCIe MSI group1 to AP */
	mmio_write_32(0x10209000, 0x00000007);

	/* Adjust EQ preset P10 coefficient */
	mmio_write_32(0x11e48038, 0x000f2100);
	mmio_write_32(0x11e48138, 0x000f2100);
	mmio_write_32(0x11e58038, 0x000f2100);

	/* CKM efuse load */
	efuse = mmio_read_32(0x11d308c4);
	intr = (efuse & GENMASK(5, 0)) << 16;
	cktx_pt0 = (efuse & GENMASK(10, 6)) >> 6;
	cktx_pt1 = (efuse & GENMASK(15, 11)) >> 11;
	/* BIAS_INTR_CTRL:  EFUSE[5:0] to 0x11f10000[21:16] */
	mmio_clrsetbits_32(0x11f10000, GENMASK(21, 16), intr);
	/* PT0_CKTX_IMPSEL: EFUSE[10:6] to 0x11f10004[4:0] */
	mmio_clrsetbits_32(0x11f10004, GENMASK(4, 0), cktx_pt0);
	/* PT1_CKTX_IMPSEL: EFUSE[15:11] to 0x11f10018[4:0] */
	mmio_clrsetbits_32(0x11f10018, GENMASK(4, 0), cktx_pt1);

	/* Disable CKM internal 50ohm pull down */
	mmio_clrbits_32(0x11f10004, BIT(5));
	mmio_clrbits_32(0x11f10018, BIT(5));

	/* Set PHY lane mode and default use 1 port 2 lane mode
	 * 0x10021110 bit[5] = 0: 1 port 2 lane
	 * 0x10021110 bit[5] = 1: 2 port 1 lane
	 */
	mmio_clrbits_32(0x10021110, BIT(5));

	/* Set PHY clock sync and default use 1 port 2 lane mode
	 * 0x11e4a088 bit[3:2] = 1 and 0x11e4a188 bit[3:2] = 1: 1 port 2 lane
	 * 0x11e4a088 bit[3:2] = 0 and 0x11e4a188 bit[3:2] = 0: 2 port 1 lane
	 */
	mmio_setbits_32(0x11e4a088, BIT(2));
	mmio_setbits_32(0x11e4a188, BIT(2));
}

void bl2_el3_plat_arch_setup(void)
{
	plat_mt_cpuxgpt_init();
	generic_delay_timer_init();
}

const struct initcall bl2_initcalls[] = {
	INITCALL(mtk_wdt_init),
	INITCALL(mtk_disable_PGD),
	INITCALL(mtk_pin_init),
	INITCALL(mt7987_set_default_pinmux),
	INITCALL(mt7987_i2c_init),
	INITCALL(mt7987_pll_init),
	INITCALL(mtk_print_cpu),
	INITCALL(mtk_infra_ao_init),
	INITCALL(mtk_pcie_init),
	INITCALL(mtk_mem_init),
	INITCALL(mtk_wed_init),
	INITCALL(mtk_i2p5g_phy_init),
	INITCALL(mt7987_enable_lvts_hw_reset),

	INITCALL(NULL)
};
