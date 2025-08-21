// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <mcucfg.h>
#include <platform_def.h>
#include <common/debug.h>
#include "pll.h"

#define aor(v, a, o)		  (((v) & (a)) | (o))
#define VDNR_DCM_TOP_INFRA_PAR_BUS_u_INFRA_PAR_BUS_CTRL_0 0x1A020004
#define INFRASYS_BUS_DCM_CTRL	  0x10001004

#define ACLKEN_DIV	0x10400640
#define BUS_PLL_DIVIDER 0x104007C0

static unsigned int _mtk_get_cpu_freq(uint32_t target_value)
{
	unsigned int temp, clk26cali0, clkdbg_cfg, clk_misc_cfg0;
	unsigned int cal_cnt, cal_cnt_valid, buspll_divider;

	buspll_divider = mmio_read_32(BUS_PLL_DIVIDER);
	mmio_write_32(BUS_PLL_DIVIDER, 0xe0201); // set BUS_PLL_DIVIDER (buspll_divider & 0xFFFFF900) | 0x200 | 0x01)

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
	mmio_write_32(BUS_PLL_DIVIDER, buspll_divider);

	return cal_cnt;
}

unsigned int mtk_get_cpu_freq(void)
{
	unsigned int ret1, ret2;

	ret1 = _mtk_get_cpu_freq(0x320); // 1.6G/2
	if (ret1 > 0)
		return ret1 * 2;

	ret2 = _mtk_get_cpu_freq(0x3e8); // 2.0G/2
	if (ret2 > 0)
		return ret2 * 2;

	return 0;
}

void mtk_pll_init(int skip_dcm_setting)
{

	/* set msdcpll to 384M*/
	INFO("first set msdcpll 384M MSDCPLL_CON1 = 0x%x\n", 0x4CCCCCCC);
	mmio_write_32(MSDCPLL_CON1, 0x4CCCCCCC);

	/* power on PLL */
	mmio_setbits_32(ARMPLL_LL_CON3, CON0_PWR_ON);
	mmio_setbits_32(MPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(MSDCPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(NET2PLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(SGMIIPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(WEDMCUPLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(NET1PLL_CON3, CON0_PWR_ON);
	mmio_setbits_32(APLL2_CON3, CON0_PWR_ON);

	udelay(1);

	/*Disable PLL ISO */
	mmio_clrbits_32(ARMPLL_LL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(MPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(MSDCPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(NET2PLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(SGMIIPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(WEDMCUPLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(NET1PLL_CON3, CON0_ISO_EN);
	mmio_clrbits_32(APLL2_CON3, CON0_ISO_EN);

	/* Set PLL frequency */

	/* 1.023v: 2.0G */
	if (mmio_read_32(ARMPLL_LL_CON0) & 0x1) {
		INFO("Change cpu clock source to xtal\n");
		mmio_clrbits_32(BUS_PLL_DIVIDER, 0x3000800);
		mmio_clrbits_32(BUS_PLL_DIVIDER, (0x1 << 9));
		/* Disable ARMPLL_LL */
		mmio_clrbits_32(ARMPLL_LL_CON0, BIT(0));
		/* Set ARMPLL_LL frequency */
		INFO("ARMPLL_LL_CON1 = 0x%x\n", 0x64000000);
		mmio_write_32(ARMPLL_LL_CON1, 0x64000000); /* pcw = 0x64000000 */
		mmio_write_32(ARMPLL_LL_CON0, 0x00670100); /* postdiv[6:4] = 000 /1*/
	} else {
		INFO("ARMPLL_LL_CON1 = 0x%x\n", 0x64000000);
		mmio_write_32(ARMPLL_LL_CON1, 0x64000000); /* pcw = 0x64000000 */
		mmio_write_32(ARMPLL_LL_CON0, 0x00670100); /* postdiv[6:4] = 000 /1*/
	}

	/* 0.85v: 1.6G(default) */
	//INFO("default ARMPLL_LL_CON1 = 0x%x\n", mmio_read_32(ARMPLL_LL_CON1)); /* default pwc=0xA000000 postdiv[6:4] = 001 /2*/
	//mmio_write_32(ARMPLL_LL_CON0, 0x00670110);

	mmio_setbits_32(MPLL_CON0, 0xFF000122);

	mmio_setbits_32(MSDCPLL_CON0, 0x00000122);

	mmio_setbits_32(NET2PLL_CON0, 0xFF000112);
	mmio_setbits_32(SGMIIPLL_CON0, 0x00000132);
	mmio_setbits_32(WEDMCUPLL_CON0, 0x00000132);
	mmio_setbits_32(NET1PLL_CON0, 0xFF000102);
	mmio_setbits_32(APLL2_CON0, 0x00000132);

	/* Enable PLL frequency */
	mmio_setbits_32(ARMPLL_LL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MSDCPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(NET2PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(SGMIIPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(WEDMCUPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(NET1PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(APLL2_CON0, CON0_BASE_EN);

	/* Wait for PLL  stable (min delay is 20us)   */
	udelay(20);

	mmio_setbits_32(MSDCPLL_CON0, CON0_SDM_PCW_CHG);

	mmio_setbits_32(MPLL_CON0, 0x00800000);
	mmio_setbits_32(MSDCPLL_CON0, 0x00800000);
	mmio_setbits_32(NET2PLL_CON0, 0x00800000);
	mmio_setbits_32(WEDMCUPLL_CON0, 0x00800000);
	mmio_setbits_32(NET1PLL_CON0, 0x00800000);

	/* Enable Infra bus divider  */
	if (skip_dcm_setting == 0) {
		mmio_setbits_32(VDNR_DCM_TOP_INFRA_PAR_BUS_u_INFRA_PAR_BUS_CTRL_0, 0x2);
		mmio_write_32(INFRASYS_BUS_DCM_CTRL, 0x5);
	}

	/* Change CPU:BUS clock ratio to 1:2 */
	mmio_clrsetbits_32(ACLKEN_DIV, 0x1f, 0x12);

	/* Switch to ARM CA7 PLL */
	mmio_setbits_32(BUS_PLL_DIVIDER, 0x3000800);
	mmio_setbits_32(BUS_PLL_DIVIDER, (0x1 << 9)); //switch to buspll

	/*Set default MUX for topckgen */
	mmio_write_32(CLK_CFG0_SET, 0x01010101);
	mmio_write_32(CLK_CFG1_SET, 0x01000101); /* fuart_bckk sel 0 */
	mmio_write_32(CLK_CFG2_SET, 0x01010001); /* fspi_ck sel 0 */
	mmio_write_32(CLK_CFG3_SET, 0x01010101);
	mmio_write_32(CLK_CFG4_SET, 0x01010101);
	mmio_write_32(CLK_CFG5_SET, 0x01010101);
	mmio_write_32(CLK_CFG6_SET, 0x01010101);
	mmio_write_32(CLK_CFG7_SET, 0x01010101);
	mmio_write_32(CLK_CFG8_SET, 0x01010101);
	mmio_write_32(CLK_CFG9_SET, 0x01010101);
	mmio_write_32(CLK_CFG10_SET, 0x01000001); /*fckm_ref sel 0 fda_clk_xtal sel 0*/
	mmio_write_32(CLK_CFG11_SET, 0x00000101);/* ignore bit 16 and 24*/

	mmio_write_32(CLK_CFG_UPDATE0, 0x7fffffff);
	mmio_write_32(CLK_CFG_UPDATE1, 0x00007fff);

	/*Set default MUX for infra */
	mmio_write_32(MODULE_CLK_SEL_0, 0x0000c077);
	mmio_write_32(MODULE_CLK_SEL_1, 0x0000000f);
}

void mtk_pll_eth_init(void)
{
	mmio_write_32(CLK_CFG0_SET, 0x01010101);
	mmio_write_32(CLK_CFG3_SET, 0x01000000);
	mmio_write_32(CLK_CFG4_SET, 0x00000001);
	mmio_write_32(CLK_CFG6_SET, 0x01010101);
	mmio_write_32(CLK_CFG7_SET, 0x01010000);
	mmio_write_32(CLK_CFG8_SET, 0x00000101);
	mmio_write_32(CLK_CFG9_SET, 0x01010100);
	mmio_write_32(CLK_CFG10_SET, 0x01000000);
	mmio_write_32(CLK_CFG11_SET, 0x00000001);

	mmio_write_32(CLK_CFG_UPDATE0, 0x4f01800f);
	mmio_write_32(CLK_CFG_UPDATE1, 0x000030c7);
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
