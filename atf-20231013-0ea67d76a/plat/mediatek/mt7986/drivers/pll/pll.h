/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLL_H
#define PLL_H

#include <stdint.h>

/* APMIXEDSYS Register */
#define APMIXED_BASE 0x1001E000

#define AP_PLL_CON6		(APMIXED_BASE + 0x18)
#define CLKSQ_STB_CON0		(APMIXED_BASE + 0x20)
#define PLL_ISO_CON0		(APMIXED_BASE + 0x2C)

#define ARMPLL_CON0		(APMIXED_BASE + 0x200)
#define ARMPLL_CON1		(APMIXED_BASE + 0x204)
#define ARMPLL_CON2		(APMIXED_BASE + 0x208)
#define ARMPLL_PWR_CON0		(APMIXED_BASE + 0x20C)

#define NET2PLL_CON0		(APMIXED_BASE + 0x210)
#define NET2PLL_CON1		(APMIXED_BASE + 0x214)
#define NET2PLL_CON2		(APMIXED_BASE + 0x218)
#define NET2PLL_PWR_CON0	(APMIXED_BASE + 0x21C)

#define MMPLL_CON0		(APMIXED_BASE + 0x220)
#define MMPLL_CON1		(APMIXED_BASE + 0x224)
#define MMPLL_CON2		(APMIXED_BASE + 0x228)
#define MMPLL_PWR_CON0	(APMIXED_BASE + 0x22C)

#define SGMIIPLL_CON0		(APMIXED_BASE + 0x230)
#define SGMIIPLL_CON1		(APMIXED_BASE + 0x234)
#define SGMIIPLL_CON2		(APMIXED_BASE + 0x238)
#define SGMIIPLL_PWR_CON0	(APMIXED_BASE + 0x23C)

#define WEDMCUPLL_CON0		(APMIXED_BASE + 0x240)
#define WEDMCUPLL_CON1		(APMIXED_BASE + 0x244)
#define WEDMCUPLL_CON2		(APMIXED_BASE + 0x248)
#define WEDMCUPLL_PWR_CON0	(APMIXED_BASE + 0x24C)

#define NET1PLL1_CON0		(APMIXED_BASE + 0x250)
#define NET1PLL1_CON1		(APMIXED_BASE + 0x254)
#define NET1PLL1_CON2		(APMIXED_BASE + 0x258)
#define NET1PLL1_PWR_CON0		(APMIXED_BASE + 0x25C)

#define APLL2_CON0		(APMIXED_BASE + 0x278)
#define APLL2_CON1		(APMIXED_BASE + 0x27C)
#define APLL2_CON2		(APMIXED_BASE + 0x280)
#define APLL2_CON3		(APMIXED_BASE + 0x284)
#define APLL2_PWR_CON0		(APMIXED_BASE + 0x288)

#define MPLL_CON0		(APMIXED_BASE + 0x260)
#define MPLL_CON1		(APMIXED_BASE + 0x264)
#define MPLL_CON2		(APMIXED_BASE + 0x268)
#define MPLL_CON3		(APMIXED_BASE + 0x26C)
#define MPLL_PWR_CON0		(APMIXED_BASE + 0x270)

#define CON0_BASE_EN		BIT(0)
#define CON0_PWR_ON		BIT(0)
#define CON0_ISO_EN		BIT(1)
#define CON0_RST_BAR		BIT(24)
#define CON0_POST_DIV(x)	((x) << 1)
#define CON1_PCW_CHG		BIT(31)
#define UNIV_48M_EN		BIT(27)

#define SCP_AXICK_DCM_DIS_EN	BIT(0)
#define SCP_AXICK_26M_SEL_EN	BIT(4)

/* INFRASYS Register */
#define INFRA_TOP_CKMUXSEL		(INFRACFG_AO_BASE + 0x00)
#define INFRA_TOP_CKDIV1		(INFRACFG_AO_BASE + 0x08)
#define INFRA_TOP_DCMCTL		(INFRACFG_AO_BASE + 0x10)

#define INFRA_TOP_CKDIV1_SEL(x)		((x) << 0)
#define INFRA_TOP_MUX_SEL(x)		((x) << 2)
#define INFRA_TOP_DCM_EN		BIT(0)

#define INFRA_PDN_CLR0			(INFRACFG_AO_BASE + 0x44)
#define	INFRA_GLOBALCON_DCMCTL		(INFRACFG_AO_BASE + 0x50)
#define INFRA_GLOBALCON_DCMCTL_MASK	0x303
#define INFRA_GLOBALCON_DCMCTL_ON	0x303

#define INFRA_GLOBALCON_DCMDBC		(INFRACFG_AO_BASE + 0x054)
#define INFRA_GLOBALCON_DCMDBC_MASK	((0x7f << 0) | (1 << 8) | \
					(0x7f << 16) | (1 << 24))
#define INFRA_GLOBALCON_DCMDBC_ON	((0 << 0) | (1 << 8) | \
					(0 << 16) | (1 << 24))

#define INFRA_GLOBALCON_DCMFSEL		(INFRACFG_AO_BASE + 0x058)
#define INFRA_GLOBALCON_DCMFSEL_MASK	((0x7 << 0) | (0xf << 8) | \
					(0x1f << 16) | (0x1f << 24))
#define INFRA_GLOBALCON_DCMFSEL_ON	((0 << 0) | (0 << 8) | \
					(0x10 << 16) | (0x10 << 24))

/* WBSYS Mapping Register */
#define WB2AP_STATUS			(INFRACFG_AO_BASE + 0x384)
#define WB2AP_PLL_RDY			0x80

/* PERI Register */
#define PERI_PDN_CLR0			(PERICFG_BASE + 0x10)
#define PERI_PDN_CLR1			(PERICFG_BASE + 0x14)

#define EFUSE_RES_REG			(EFUSE_BASE + 0x82C)
#define EFUSE_N9_POLLING_MASK		0x3

/* Audio Register*/
#define AUDIO_TOP_CON0			(AUDIO_BASE + 0x00)
#define AUDIO_TOP_AHB_IDLE_EN		BIT(29)

#define I2S0_MCK_DIV_PDN		BIT(2)
#define APLL2_MUX_SEL_294M		BIT(7)

/* Topckgen MUX */
#define CLK_TOP_AXI_SEL_MUX_SYSPLL1_D2		(0x1 << 0)
#define CLK_TOP_MEM_SEL_MUX_DMPLL1_CK		(0x1 << 8)
#define CLK_TOP_DDRPHYCFG_SEL_MUX_SYSLL1_D8	(0x1 << 16)
#define CLK_TOP_ETH_SEL_MUX_MMPLL1_D2		(0x2 << 24)

#define CLK_TOP_PWM_SEL_MUX_MMPLL2_D4		(0x1 << 0)
#define CLK_TOP_F10M_REF_SEL_MUX_SYSPLL4_D16	(0x1 << 8)
#define CLK_TOP_NFI_INFRA_SEL_MUX_MMPLL2_D8	(0x8 << 16)
#define CLK_TOP_FLASH_SEL_MUX_SYSPLL2_D8	(0x2 << 24)

#define CLK_TOP_UART_SEL_MUX_CLKXTAL		(0x0 << 0)
#define CLK_TOP_SPI0_SEL_MUX_SYSPLL3_D2		(0x1 << 8)
#define CLK_TOP_SPI1_SEL_MUX_SYSPLL3_D2		(0x1 << 16)
#define CLK_TOP_MSDC50_0_SEL_MUX_MMPLL2_D8	(0x1 << 24)

#define CLK_TOP_MSDC30_0_SEL_MUX_UNIV48M	(0x2 << 0)
#define CLK_TOP_MSDC30_1_SEL_MUX_UNIV48M	(0x2 << 8)
#define CLK_TOP_A1SYS_HP_SEL_MUX_AUD1PLL_294P912M	(0x1 << 16)
#define CLK_TOP_A2SYS_HP_SEL_MUX_AUD2PLL_270P504M	(0x2 << 24)

#define CLK_TOP_INTDIR_SEL_MUX_MMPLL_D2	(0x2 << 0)
#define CLK_TOP_AUD_INTBUS_SEL_MUX_SYSPLL1_D4	(0x1 << 8)
#define CLK_TOP_PMICSPI_SEL_MUX_MMPLL2_D16	(0x5 << 16)
#define CLK_TOP_SCP_SEL_MUX_SYSPLL1_D8		(0x1 << 24)

#define CLK_TOP_ATB_SEL_MUX_SYSPLL_D5		(0x1 << 0)
#define CLK_TOP_HIF_SEL_MUX_SYSPLL1_D2		(0x1 << 8)
#define CLK_TOP_AUDIO_SEL_MUX_SYSPLL3_D4	(0x1 << 16)
#define CLK_TOP_U2_SEL_MUX_SYSPLL1_D8		(0x1 << 24)

#define CLK_TOP_AUD1_SEL_MUX_AUD1PLL_294P912M	(0x1 << 0)
#define CLK_TOP_AUD2_SEL_MUX_AUD2PLL_270P9504M	(0x1 << 8)
#define CLK_TOP_IRRX_SEL_MUX_SYSPLL4_D16	(0x1 << 16)
#define CLK_TOP_IRTX_SEL_MUX_SYSPLL4_D16	(0x1 << 24)

#define CLK_TOP_ASM_L_SEL_MUX_MMPLL2_D4	(0x3 << 0)
#define CLK_TOP_ASM_M_SEL_MUX_MMPLL2_D2	(0x2 << 8)
#define CLK_TOP_ASM_H_SEL_MUX_SYSPLL_D5		(0x1 << 16)

enum {
	DDR533	= 533,
	DDR667	= 667,
	DDR800	= 800,
	DDR938	= 938,
	DDR1066	= 1066,
	DDR1333	= 1333,
	DDR1466	= 1466,
	DDR1600	= 1600,
};

#define CLK_CFG_0   0x1001B004
#define CLK_CFG_1   0x1001B014
#define CLK_CFG_2   0x1001B024
#define CLK_CFG_3   0x1001B034
#define CLK_CFG_4   0x1001B044
#define CLK_CFG_5   0x1001B054
#define CLK_CFG_6   0x1001B064
#define CLK_CFG_7   0x1001B074
#define CLK_CFG_8   0x1001B084
#define CLK_CFG_9   0x1001B094
#define CLK_CFG_20  0x1001B144

uint32_t mt_get_ckgen_ck_freq(uint32_t id);
void mtk_pll_init(int);

#endif
