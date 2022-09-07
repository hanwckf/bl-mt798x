/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <mcucfg.h>
#include <platform_def.h>
#include <common/debug.h>
#include "pll.h"

#define aor(v, a, o)			(((v) & (a)) | (o))
#define VDNR_DCM_TOP_INFRA_CTRL_0	0x1A02003C
#define INFRASYS_BUS_DCM_CTRL		0x10001004
#define INFRACFG_HANG_FREE_DEBUG	0x100030f0

#define ACLKEN_DIV  			0x10400640
#define BUS_PLL_DIVIDER 		0x104007c0

uint32_t mt_get_ckgen_ck_freq(uint32_t id)
{
	unsigned int temp, clk26cali_0, clk_cfg_9, clk_misc_cfg_1;
	int i = 0;

	mmio_write_32(0x104007c0, 0xe0201);

	clk26cali_0 = mmio_read_32(0x1001B320);

	clk_misc_cfg_1 = mmio_read_32(0x1001B200);
	mmio_write_32(0x1001B200, 0x0);

	clk_cfg_9 = mmio_read_32(0x1001B240);
	mmio_write_32(0x1001B240, 0x00040000);

	temp = mmio_read_32(0x1001B324);
	mmio_write_32(0x1001B324, 0x270000);

	mmio_write_32(0x1001B320, 0x1010);

	/* wait frequency meter finish */
	mdelay(100);

	temp = mmio_read_32(0x1001B324) & 0xFFFF;

	mmio_write_32(0x1001B240, clk_cfg_9);
	mmio_write_32(0x1001B200, clk_misc_cfg_1);
	mmio_write_32(0x1001B320, clk26cali_0);

	if (i > 10)
		return 0;

	return temp;
}

void mtk_pll_init(int skip_dcm_setting)
{

	/* Power on PLL */
	mmio_setbits_32(ARMPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(NET2PLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(MMPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(SGMIIPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(WEDMCUPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(NET1PLL1_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(APLL2_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(MPLL_PWR_CON0, CON0_PWR_ON);

	udelay(1);

	/* Disable PLL ISO */
	mmio_clrbits_32(ARMPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(NET2PLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(MMPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(SGMIIPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(WEDMCUPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(NET1PLL1_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(APLL2_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(MPLL_PWR_CON0, CON0_ISO_EN);

	/* Set PLL frequency */
	if ((mmio_read_32(IAP_REBB_SWITCH) & IAP_IND) == 0) {
		mmio_write_32(ARMPLL_CON1, 0x50000000); /* 1.6G */
	}
	else {
		if ((mmio_read_32(BS_PAD_SYS_WATCHDOG) & BS_CPU_VCORE))
			mmio_write_32(ARMPLL_CON1, 0x64000000); /* 1.023: 2G */
		else
			mmio_write_32(ARMPLL_CON1, 0x50000000); /* 0.85: 1.6G */
	}

	mmio_setbits_32(ARMPLL_CON0, 0x104); /* 1.6G or 2G */

	mmio_setbits_32(NET2PLL_CON0, 0x114);
	mmio_setbits_32(MMPLL_CON0, 0x114);
	mmio_setbits_32(SGMIIPLL_CON0, 0x134);
	mmio_setbits_32(WEDMCUPLL_CON0, 0x114);
	mmio_setbits_32(NET1PLL1_CON0, 0x104);
	mmio_setbits_32(APLL2_CON0, 0x134);
	mmio_setbits_32(MPLL_CON0, 0x124);

	/* Enable PLL frequency */
	mmio_setbits_32(ARMPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(NET2PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MMPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(SGMIIPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(WEDMCUPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(NET1PLL1_CON0, CON0_BASE_EN);
	mmio_setbits_32(APLL2_CON0, CON0_BASE_EN);	/* 750MHz */
	mmio_setbits_32(MPLL_CON0, CON0_BASE_EN);	/* 650MHz */

	/* Wait for PLL stable (min delay is 20us) */
	udelay(20);

	mmio_setbits_32(NET2PLL_CON0, 0x1000000);
	mmio_setbits_32(MMPLL_CON0, 0x1000000);
	mmio_setbits_32(WEDMCUPLL_CON0, 0x1000000);
	mmio_setbits_32(NET1PLL1_CON0, 0x1000000);
	mmio_setbits_32(MPLL_CON0, 0x01000000);

	/* Enable Infra bus divider */
	if (skip_dcm_setting == 0) {
		mmio_setbits_32(VDNR_DCM_TOP_INFRA_CTRL_0, 0x2);
		mmio_write_32(INFRASYS_BUS_DCM_CTRL, 0x5);
	}

	/* Change CPU:CCI clock ratio to 1:2 */
	mmio_clrsetbits_32(ACLKEN_DIV, 0x1f, 0x12);

	/* Switch to ARM CA7 PLL */
	mmio_setbits_32(BUS_PLL_DIVIDER, 0x3000800);
	mmio_setbits_32(BUS_PLL_DIVIDER, (0x1 << 9));

	/* Set default MUX for topckgen */
	mmio_write_32(CLK_CFG_0, 0x00000101);
	mmio_write_32(CLK_CFG_1, 0x01010100);
	mmio_write_32(CLK_CFG_2, 0x01010000);
	mmio_write_32(CLK_CFG_3, 0x01010101);
	mmio_write_32(CLK_CFG_4, 0x01010100);
	mmio_write_32(CLK_CFG_5, 0x01010101);
	mmio_write_32(CLK_CFG_6, 0x01010101);
	mmio_write_32(CLK_CFG_7, 0x01010101);
	mmio_write_32(CLK_CFG_8, 0x01010101);
	mmio_write_32(CLK_CFG_9, 0x00000001);

	mmio_write_32(0x1001B1C0, 0x7FFEFCE3);
	mmio_write_32(0x1001B1C4, 0x1F);
	mmio_setbits_32(0x10001010, 0x1);
	mmio_write_32(0x10001050, 0x1 << 20);
	mmio_write_32(0x10001050, 0x1 << 21);
	/* Enable infra cfg to conctrol msdc cg */
	mmio_clrbits_32(INFRACFG_HANG_FREE_DEBUG, (0xf << 4));

#ifndef IMAGE_BL2PL
	NOTICE("CPU: MT%x (%dMHz)\n", SOC_CHIP_ID, mt_get_ckgen_ck_freq(0) * 2);
#endif
}
