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

#define aor(v, a, o)		  (((v) & (a)) | (o))
#define VDNR_DCM_TOP_INFRA_CTRL_0 0x1A020018
#define INFRASYS_BUS_DCM_CTRL	  0x10001004

#define ACLKEN_DIV	0x100E0640
#define BUS_PLL_DIVIDER 0x100E07C0
#define ARM_PLL_DIVIDER 0x100E07A8

static unsigned int _mtk_get_cpu_freq(uint32_t target_value)
{
	unsigned int temp, clk26cali0, clkdbg_cfg, clk_misc_cfg0;
	unsigned int cal_cnt, cal_cnt_valid, armpll_divider;

	armpll_divider = mmio_read_32(ARM_PLL_DIVIDER);
	mmio_write_32(ARM_PLL_DIVIDER, 0xe0201); // set ARM_PLL_DIVIDER

	clk26cali0 = mmio_read_32(CLK_26CALI_0);
	clk_misc_cfg0 = mmio_read_32(CLK_MISC_CFG_0);
	mmio_write_32(CLK_MISC_CFG_0, 0x0);
	clkdbg_cfg = mmio_read_32(CLK_DBG_CFG);
	mmio_write_32(CLK_DBG_CFG, 0x00040000);
	mmio_write_32(CLK_26CALI_1, (0x27 << 20 | 0x2 << 16 | target_value));
	mmio_write_32(CLK_26CALI_0, 0x101);

	temp = mmio_read_32(CLK_26CALI_0);
	cal_cnt = temp & 0x1;
	while (cal_cnt != 0) {
		temp = mmio_read_32(CLK_26CALI_0);
		cal_cnt = temp & 0x1;
	}
	/* wait frequency meter finish */
	mdelay(100);
	cal_cnt = ((temp & 0xffff0000) >> 16);
	cal_cnt_valid = ((temp & 0x00001000) >> 12);
	if ((cal_cnt < (target_value - 2)) || (cal_cnt > (target_value + 2)) ||
	    (cal_cnt_valid == 0)) {
		NOTICE("Unexpected CPU frequency measured\n");
	}

	mmio_write_32(CLK_DBG_CFG, clkdbg_cfg);
	mmio_write_32(CLK_MISC_CFG_0, clk_misc_cfg0);
	mmio_write_32(CLK_26CALI_0, clk26cali0);
	mmio_write_32(ARM_PLL_DIVIDER, armpll_divider);

	return cal_cnt;
}

unsigned int mtk_get_cpu_freq(void)
{
	unsigned int ret1, ret2, ret3;

	ret1 = _mtk_get_cpu_freq(0x384); // 1.8G/2
	if (ret1 > 0)
		return ret1 * 2;

	ret2 = _mtk_get_cpu_freq(0x2ee); // 1.5G/2
	if (ret2 > 0)
		return ret2 * 2;

	ret3 = _mtk_get_cpu_freq(0x190); // 800M/2
	if (ret3 > 0)
		return ret3 * 2;

	return 0;
}

void mtk_pll_init(int skip_dcm_setting)
{

	/* power on PLL */
	mmio_setbits_32(ARMPLL_B_CON3, CON0_PWR_ON);
	mmio_setbits_32(CCIPLL2_B_CON3, CON0_PWR_ON);
	mmio_setbits_32(NETSYSPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(MSDCPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(NET2PLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(MMPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(SGMIIPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(WEDMCUPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(NET1PLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(MPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(APLL2_CON3, CON0_PWR_ON);
	mmio_setbits_32(ARMPLL_RSV_CON3, CON0_PWR_ON);

	udelay(1);

	/*Disable PLL ISO */
	mmio_clrbits_32(ARMPLL_B_CON3, CON0_ISO_EN);
	mmio_clrbits_32(CCIPLL2_B_CON3, CON0_ISO_EN);
	mmio_clrbits_32(NETSYSPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(MSDCPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(NET2PLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(MMPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(SGMIIPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(WEDMCUPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(NET1PLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(MPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(APLL2_CON3, CON0_ISO_EN);
	mmio_clrbits_32(ARMPLL_RSV_CON3, CON0_ISO_EN);

	/* Set PLL frequency */
	/* please change Vproc to 900mv via i2c */

#ifdef CPU_USE_FULL_SPEED
	INFO("ARMPLL_B_CON1 = 0x%x\n", 0xB4000000);
	mmio_write_32(ARMPLL_B_CON1, 0xB4000000);	/* 1.8G */
#else
	INFO("ARMPLL_B_CON1 = 0x%x\n", 0x96000000);
	mmio_write_32(ARMPLL_B_CON1, 0x96000000);	/* 1.5G */
#endif
	//mmio_write_32(ARMPLL_B_CON1, 0x50000000);	/* 800M */

	mmio_setbits_32(ARMPLL_B_CON0, 0x00000114);

#ifdef CPU_USE_FULL_SPEED
	mmio_write_32(CCIPLL2_B_CON1, 0x6C000000); /*1060M */
#else
	mmio_write_32(CCIPLL2_B_CON1, 0x5A000000); /*900M */
#endif

	mmio_setbits_32(CCIPLL2_B_CON0, 0x00000114);

	mmio_setbits_32(NETSYSPLL_CON0, 0x00000114);
	mmio_setbits_32(MSDCPLL_CON0, 0x00000124);
	mmio_setbits_32(NET2PLL_CON0, 0xEA000114);
	mmio_setbits_32(MMPLL_CON0, 0x00000124);
	mmio_setbits_32(SGMIIPLL_CON0, 0x00000134);
	mmio_setbits_32(WEDMCUPLL_CON0, 0x00000134);
	mmio_setbits_32(NET1PLL_CON0, 0x00000104);
	mmio_setbits_32(MPLL_CON0, 0x00000124);
	mmio_setbits_32(APLL2_CON0, 0x00000134);
	mmio_setbits_32(ARMPLL_RSV_CON0, 0x00000124);

	/* Enable PLL frequency */
	mmio_setbits_32(ARMPLL_B_CON0, CON0_BASE_EN);
	mmio_setbits_32(CCIPLL2_B_CON0, CON0_BASE_EN);
	mmio_setbits_32(NETSYSPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MSDCPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(NET2PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MMPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(SGMIIPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(WEDMCUPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(NET1PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(APLL2_CON0, CON0_BASE_EN);
	mmio_setbits_32(ARMPLL_RSV_CON0, CON0_BASE_EN);

	/* Wait for PLL  stable (min delay is 20us)   */
	udelay(20);

	mmio_setbits_32(CCIPLL2_B_CON0, 0x00800000);
	mmio_setbits_32(NETSYSPLL_CON0, 0x00800000);
	mmio_setbits_32(MSDCPLL_CON0, 0x00800000);
	mmio_setbits_32(NET2PLL_CON0, 0x00800000);
	mmio_setbits_32(MMPLL_CON0, 0x00800000);
	mmio_setbits_32(WEDMCUPLL_CON0, 0x00800000);
	mmio_setbits_32(NET1PLL_CON0, 0x00800000);
	mmio_setbits_32(MPLL_CON0, 0x00800000);

	/* Enable Infra bus divider  */
	if (skip_dcm_setting == 0) {
		mmio_setbits_32(VDNR_DCM_TOP_INFRA_CTRL_0, 0x0400);
		mmio_write_32(INFRASYS_BUS_DCM_CTRL, 0x5);
	}

	mmio_setbits_32(ARM_PLL_DIVIDER, (0x1 << 9)); //switch to armpll
	mmio_setbits_32(BUS_PLL_DIVIDER, (0x1 << 9)); //switch to buspll

	/*Set default MUX for topckgen */
	mmio_write_32(CLK_CFG0_SET, 0x01010101);
	mmio_write_32(CLK_CFG1_SET, 0x01020101);
	mmio_write_32(CLK_CFG2_SET, 0x01010001);
	mmio_write_32(CLK_CFG3_SET, 0x02010101);
	mmio_write_32(CLK_CFG4_SET, 0x01010101);
	mmio_write_32(CLK_CFG5_SET, 0x01010101);
	mmio_write_32(CLK_CFG6_SET, 0x01010101);
	mmio_write_32(CLK_CFG7_SET, 0x01010101);
	mmio_write_32(CLK_CFG8_SET, 0x01010101);
	mmio_write_32(CLK_CFG9_SET, 0x01010101);
	mmio_write_32(CLK_CFG10_SET, 0x01010101);
	mmio_write_32(CLK_CFG11_SET, 0x01010101);
	mmio_write_32(CLK_CFG12_SET, 0x01010101);
	mmio_write_32(CLK_CFG13_SET, 0x01010101);
	mmio_write_32(CLK_CFG14_SET, 0x01010101);
	mmio_write_32(CLK_CFG15_SET, 0x00000101);
	mmio_write_32(CLK_CFG16_SET, 0x01010100);
	mmio_write_32(CLK_CFG17_SET, 0x01010101);
	mmio_write_32(CLK_CFG18_SET, 0x00000101);

	mmio_write_32(CLK_CFG_UPDATE0, 0x7fffffff);
	mmio_write_32(CLK_CFG_UPDATE1, 0x7fffffff);
	mmio_write_32(CLK_CFG_UPDATE2, 0x00000ff8);

	/*Set default MUX for infra */
	mmio_write_32(MODULE_CLK_SEL_0, 0xffffc077);
	mmio_write_32(MODULE_CLK_SEL_1, 0x000000ff);

        /*power on audio sram, 0: power on, 1: power off*/
	mmio_clrbits_32(AUD_SRAM_PD_CTRL, AUD_SRAM_PWR_ON);
}

void mtk_pll_eth_init(void)
{
	mmio_write_32(CLK_CFG0_SET, 0x01010101);
	mmio_write_32(CLK_CFG1_SET, 0x01020101);
	mmio_clrsetbits_32(CLK_CFG4_SET, 0xff000000, 0x01000000);
	mmio_clrsetbits_32(CLK_CFG5_SET, 0x00ffffff, 0x00010101);
	mmio_clrsetbits_32(CLK_CFG8_SET, 0xff000000, 0x01000000);
	mmio_write_32(CLK_CFG9_SET, 0x01010101);
	mmio_clrsetbits_32(CLK_CFG10_SET, 0x00ffffff, 0x00010101);
	mmio_clrsetbits_32(CLK_CFG11_SET, 0xffffff00, 0x01010100);
	mmio_clrsetbits_32(CLK_CFG12_SET, 0x000000ff, 0x00000001);
	mmio_clrsetbits_32(CLK_CFG13_SET, 0xffff0000, 0x01010000);
	mmio_clrsetbits_32(CLK_CFG14_SET, 0x0000ffff, 0x00000101);
	mmio_clrsetbits_32(CLK_CFG16_SET, 0xff0000ff, 0x01000000);
	mmio_clrsetbits_32(CLK_CFG17_SET, 0xffff0000, 0x01010000);
	mmio_clrsetbits_32(CLK_CFG18_SET, 0x000000ff, 0x00000001);

	mmio_write_32(CLK_CFG_UPDATE0, 0x7800ff);
	mmio_write_32(CLK_CFG_UPDATE1, 0x3C1E7F8);
	mmio_write_32(CLK_CFG_UPDATE2, 0x1C9);
}

void eth_2p5g_phy_mtcmos_ctrl(int enable)
{
	int pwr_ack, pwr_ack_2nd;
	int count;

	if (enable) {
		mmio_setbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_ON);
		count = 0;
		pwr_ack = mmio_read_32(ETH_2P5G_PHY_DOM_CON) &
			  ETH_2P5G_PHY_PWR_ACK;
		while (!pwr_ack) {
			if (count >= MAX_POLLING_CNT)
				break;
			pwr_ack = mmio_read_32(ETH_2P5G_PHY_DOM_CON) &
				  ETH_2P5G_PHY_PWR_ACK;
			count++;
			udelay(1);
		}

		count = 0;
		mmio_setbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_ON_2ND);
		pwr_ack_2nd = mmio_read_32(ETH_2P5G_PHY_DOM_CON) &
			      ETH_2P5G_PHY_PWR_ACK_2ND;
		while (!pwr_ack_2nd) {
			if (count >= MAX_POLLING_CNT)
				break;
			pwr_ack_2nd = mmio_read_32(ETH_2P5G_PHY_DOM_CON) &
				      ETH_2P5G_PHY_PWR_ACK_2ND;
			count++;
			udelay(1);
		}

		udelay(30);
		mmio_clrbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_ISO);
		udelay(30);

		mmio_setbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_RST_N);
		mmio_clrbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_CLK_DIS);
		mmio_clrbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_SRAM_PDN);
	} else {
		mmio_setbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_SRAM_PDN);
		mmio_setbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_CLK_DIS);
		mmio_setbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_ISO);
		udelay(30);

		mmio_clrbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_RST_N);
		mmio_clrbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_ON);
		mmio_clrbits_32(ETH_2P5G_PHY_DOM_CON, ETH_2P5G_PHY_PWR_ON_2ND);
	}
}
