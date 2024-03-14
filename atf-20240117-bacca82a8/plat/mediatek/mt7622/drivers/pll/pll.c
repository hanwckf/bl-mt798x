/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <mcucfg.h>
#include <mtcmos.h>
#include <platform_def.h>
#include <spm.h>
#include "pll.h"

#define aor(v, a, o)			(((v) & (a)) | (o))

#define pllc1_mempll_div_6_0		(0x52 << 17)
#define pllc1_mempll_reserve_1		(0 << 0)
#define pllc1_mempll_reserve		BIT(26)
#define pllc1_mempll_div_en		BIT(16)
#define pllc1_mempll_n_info_chg		BIT(31)
#define pllc1_mempll_bias_en		BIT(6)
#define pllc1_mempll_bias_lpf_en	BIT(7)
#define pllc1_mempll_en			BIT(28)
#define pllc1_mempll_sdm_prd_1		BIT(30)

#define mempll_prediv_1_0		BIT(14)
#define mempll_br			BIT(10)
#define mempll_bp			BIT(12)
#define mempll_fbksel_1_0		(0 << 24)
#define mempll_posdiv_1_0		(0 << 13)
#define mempll_ref_dl_4_0		(0 << 8)
#define mempll_fb_dl_4_0		(0 << 0)
#define mempll_en			BIT(0)

static void mtk_mempll_init(int freq)
{
	int pllc1_mempll_n_info_30_24_n4;
	int pllc1_mempll_n_info_23_0_n4;

	int mempll_vco_div_sel = 0;
	int mempll_m4pdiv_1_0 = 0;
	int mempll_fbdiv_6_0;
	int mempll_fb_mck_sel = 0;

	/* (0) Set MEM25M_OUT_EN */
	mmio_setbits_32(APMIXED_BASE + 0x48, BIT(4));

	/* (1) Setup DDRPHY operation mode */

	/* DQIEN deglitch enable */
	mmio_setbits_32(DDRPHY_BASE + 0x0c, BIT(22) | BIT(23));
	/* DFREQ_DIV2 */
	mmio_setbits_32(DRAMC0_BASE + 0x7c, BIT(0));
	mmio_setbits_32(DDRPHY_BASE + 0x7c, BIT(0));

	/* for 1pll mode duty */
	mmio_write_32(DDRPHY_BASE + 0xcc, BIT(2) | BIT(3));

	mmio_write_32(DDRPHY_BASE + 0x648, 0x02001819);
	mmio_setbits_32(DDRPHY_BASE + 0x64c, BIT(0) | BIT(3));

	/* (2) Setup MEMPLL operation case & frequency */
	switch (freq) {
	case 533:
		pllc1_mempll_n_info_30_24_n4	= 0x00000051 << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x005040c6 << 0;
		mempll_fbdiv_6_0		= 0x2b << 16;
		break;
	case 667:
		pllc1_mempll_n_info_30_24_n4	= 0x00000051 << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x000734b7 << 0;
		mempll_fbdiv_6_0		= 0x1b << 16;
		break;
	case 800:
		pllc1_mempll_n_info_30_24_n4	= 0x0000004f << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x0083e0f8 << 0;
		mempll_fbdiv_6_0		= 0x21 << 16;
		break;
	case 1066:
		pllc1_mempll_n_info_30_24_n4	= 0x00000051 << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x005040c6 << 0;
		mempll_fbdiv_6_0		= 0x2b << 16;
		break;
	case 1333:
		pllc1_mempll_n_info_30_24_n4	= 0x00000050 << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x00f7a803 << 0;
		mempll_fbdiv_6_0		= 0x36 << 16;
		break;
	case 1466:
		pllc1_mempll_n_info_30_24_n4	= 0x00000051 << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x007fe9c8 << 0;
		mempll_fbdiv_6_0		= 0x3b << 16;
		break;
	case 1600:
		pllc1_mempll_n_info_30_24_n4	= 0x00000050 << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x00bd0bd0 << 0;
		mempll_fbdiv_6_0		= 0x41 << 16;
		break;
	default:/* 533 */
		pllc1_mempll_n_info_30_24_n4	= 0x0000004f << 24;
		pllc1_mempll_n_info_23_0_n4	= 0x00772807 << 0;
		mempll_fbdiv_6_0		= 0x16 << 16;
		break;
	}

	if (freq == 533)
		mempll_m4pdiv_1_0 = BIT(28);

	if (freq < 1333)
		mempll_vco_div_sel = BIT(14);

	/* (2) Setup MEMPLL operation case & frequency */

	/* default val: [0x0600] CK1600 XTAL_25M */
	mmio_write_32(DDRPHY_BASE + 0x600, BIT(30) | BIT(31));
	mmio_write_32(DDRPHY_BASE + 0x614, BIT(20) | BIT(23) | BIT(26));
	mmio_write_32(DDRPHY_BASE + 0x620, BIT(20) | BIT(23) | BIT(26));
	mmio_write_32(DDRPHY_BASE + 0x62C, BIT(20) | BIT(23) | BIT(26));

	/* mempll1 setting P1 P2 P3 */
	mmio_write_32(DDRPHY_BASE + 0x604, BIT(19) | BIT(21));
	mmio_write_32(DDRPHY_BASE + 0x60c, pllc1_mempll_reserve_1);
	mmio_write_32(DDRPHY_BASE + 0x610,
		      pllc1_mempll_div_6_0 | pllc1_mempll_reserve);

	/* mempll2 */
	mmio_write_32(DDRPHY_BASE + 0x614,
		      mempll_fbdiv_6_0 | mempll_prediv_1_0 |
		      mempll_posdiv_1_0 | mempll_fbksel_1_0);
	mmio_write_32(DDRPHY_BASE + 0x618,
		      mempll_m4pdiv_1_0 | mempll_fb_mck_sel |
		      mempll_ref_dl_4_0 | mempll_fb_dl_4_0);
	mmio_write_32(DDRPHY_BASE + 0x61c,
		      mempll_vco_div_sel | mempll_br | mempll_bp);

	/* mempll3 */
	mmio_write_32(DDRPHY_BASE + 0x620,
		      mempll_fbdiv_6_0 | mempll_prediv_1_0 |
		      mempll_posdiv_1_0 | mempll_fbksel_1_0);
	mmio_write_32(DDRPHY_BASE + 0x624,
		      mempll_m4pdiv_1_0 | mempll_fb_mck_sel |
		      mempll_ref_dl_4_0 | mempll_fb_dl_4_0);
	mmio_write_32(DDRPHY_BASE + 0x628,
		      mempll_vco_div_sel | mempll_br | mempll_bp);

	/* mempll4 */
	mmio_write_32(DDRPHY_BASE + 0x62c,
		      mempll_fbdiv_6_0 | mempll_prediv_1_0 |
		      mempll_posdiv_1_0 | mempll_fbksel_1_0);
	mmio_write_32(DDRPHY_BASE + 0x630,
		      mempll_m4pdiv_1_0 | mempll_fb_mck_sel |
		      mempll_ref_dl_4_0 | mempll_fb_dl_4_0);
	mmio_write_32(DDRPHY_BASE + 0x634,
		      mempll_vco_div_sel | mempll_br | mempll_bp);

	/* mempll1 */
	mmio_write_32(DDRPHY_BASE + 0x600,
		      pllc1_mempll_n_info_23_0_n4 |
		      pllc1_mempll_n_info_30_24_n4 |
		      pllc1_mempll_n_info_chg);
	/* tongle 600[31] for info change */
	mmio_clrbits_32(DDRPHY_BASE + 0x0600, BIT(31));

	/* (3) Setup MEMPLL power on sequence */
	udelay(2);

	mmio_write_32(DDRPHY_BASE + 0x60c,
		      pllc1_mempll_reserve_1 | pllc1_mempll_bias_en |
		      pllc1_mempll_bias_lpf_en);
	udelay(1000);

	mmio_write_32(DDRPHY_BASE + 0x604,
		      BIT(19) | BIT(21) | pllc1_mempll_en);
	udelay(20);

	mmio_write_32(DDRPHY_BASE + 0x60c,
		      pllc1_mempll_reserve_1 | pllc1_mempll_bias_en |
		      pllc1_mempll_bias_lpf_en | pllc1_mempll_sdm_prd_1);
	udelay(2);

	mmio_write_32(DDRPHY_BASE + 0x604,
		      BIT(19) | BIT(21) | pllc1_mempll_en);
	mmio_write_32(DDRPHY_BASE + 0x610,
		      pllc1_mempll_div_en | pllc1_mempll_div_6_0 |
		      pllc1_mempll_reserve);
	udelay(23);

	mmio_write_32(DDRPHY_BASE + 0x614,
		      mempll_en | mempll_fbdiv_6_0 | mempll_prediv_1_0 |
		      mempll_posdiv_1_0 | mempll_fbksel_1_0);
	mmio_write_32(DDRPHY_BASE + 0x620,
		      mempll_fbdiv_6_0 | mempll_prediv_1_0 |
		      mempll_posdiv_1_0 | mempll_fbksel_1_0);
	mmio_write_32(DDRPHY_BASE + 0x62c,
		      mempll_en | mempll_fbdiv_6_0 | mempll_prediv_1_0 |
		      mempll_posdiv_1_0 | mempll_fbksel_1_0);

	udelay(23);

	/* MEMPLL: RG_DMALL_CK_EN */
	mmio_setbits_32(DDRPHY_BASE + 0x640, BIT(4));

	/* bypass spm 0x648[25] */
	mmio_setbits_32(DDRPHY_BASE + 0x648, BIT(25));
	mmio_write_32(DDRPHY_BASE + 0x650, BIT(0) | BIT(3) | BIT(4));
}

static const unsigned int wait_time[] = {30, 0, 5, 200};

static unsigned int efuse_pll_waitime(void)
{
	return mmio_read_32(EFUSE_RES_REG) & EFUSE_N9_POLLING_MASK;
}

static int wait_n9_cali(void)
{
	unsigned int us = 30, idx = 0;

	idx = efuse_pll_waitime();
	if (idx)
		us = wait_time[idx];

	if (us > 0 && !(mmio_read_32(WB2AP_STATUS) & WB2AP_PLL_RDY)) {
		unsigned int timeout = us * 2000;

		while (timeout--)
			if (mmio_read_32(WB2AP_STATUS) & WB2AP_PLL_RDY)
				break;

		if (!(mmio_read_32(WB2AP_STATUS) & WB2AP_PLL_RDY))
			return -1;
	}

	return 0;
}

void mtk_pll_init(void)
{
	int ret = 0;
	unsigned int temp;

	ret = wait_n9_cali();
	if (ret)
		return;

	mmio_write_32((uintptr_t)&mt7622_mcucfg->aclken_div, MCU_BUS_DIV2);

	/* Reduce CLKSQ disable time */
	mmio_write_32(CLKSQ_STB_CON0, 0x98940501);

	/* Extend PWR/ISO control timing to 1 us */
	mmio_write_32(PLL_ISO_CON0, BIT(3) | BIT(19));
	mmio_write_32(AP_PLL_CON6, 0);

	mmio_setbits_32(UNIVPLL_CON0, UNIV_48M_EN);

	/* Power on PLL */
	mmio_setbits_32(ARMPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(MAINPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(UNIVPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(APLL1_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(APLL2_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(ETH1PLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(ETH2PLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(TRGPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(SGMIPLL_PWR_CON0, CON0_PWR_ON);

	udelay(1);

	/* Disable PLL ISO */
	mmio_clrbits_32(ARMPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(MAINPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(UNIVPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(APLL1_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(APLL2_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(ETH1PLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(ETH2PLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(TRGPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(SGMIPLL_PWR_CON0, CON0_ISO_EN);

	/* Set PLL frequency */
	if (!((mmio_read_32(RGU_STRAP_PAR) & RGU_INPUT_TYPE) >> 1)) {
		mmio_write_32(ARMPLL_CON1, 0x800D8000);		// 1350MHz
		mmio_write_32(MAINPLL_CON1, 0x800B3333);	// 1120MHz
		mmio_write_32(UNIVPLL_CON1, 0x80180000);	// 1200MHz
		mmio_write_32(APLL1_CON1, 0xAF2F9873);		// 294.912MHz
		mmio_write_32(APLL2_CON1, 0xAB5A1CAB);		// 270.95MHz
		mmio_write_32(ETH1PLL_CON1, 0x800A0000);	// 500MHz
		mmio_write_32(ETH2PLL_CON1, 0x800D0000);	// 650MHz
		mmio_write_32(TRGPLL_CON1, 0x800E8000);		// 725MHz
		mmio_write_32(SGMIPLL_CON1, 0x800D0000);	// 650MHz
	} else {
		mmio_write_32(ARMPLL_CON1, 0x8010E000);		// 1350MHz
		mmio_write_32(MAINPLL_CON1, 0x800E0000);	// 1120MHz
		mmio_write_32(UNIVPLL_CON1, 0x801E0000);	// 1200MHz
		mmio_write_32(APLL1_CON1, 0xBB000000);		// 294.912MHz
		mmio_write_32(APLL2_CON1, 0xB6000000);		// 270.95MHz
		mmio_write_32(ETH1PLL_CON1, 0x800C8000);	// 500MHz
		mmio_write_32(ETH2PLL_CON1, 0x80104000);	// 650MHz
		mmio_write_32(TRGPLL_CON1, 0x80122000);		// 725MHz
		mmio_write_32(SGMIPLL_CON1, 0x80104000);	// 650MHz
	}

	/* Set Audio PLL post divider */
	mmio_setbits_32(APLL1_CON0, CON0_POST_DIV(0x3));
	mmio_setbits_32(APLL2_CON0, CON0_POST_DIV(0x3));

	/* Enable PLL frequency */
	mmio_setbits_32(ARMPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MAINPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(UNIVPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(APLL1_CON0, CON0_BASE_EN);
	mmio_setbits_32(APLL2_CON0, CON0_BASE_EN);
	mmio_setbits_32(ETH1PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(ETH2PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(TRGPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(SGMIPLL_CON0, CON0_BASE_EN);

	/* Wait for PLL stable (min delay is 20us) */
	udelay(20);

	/* PLL DIV RSTB */
	mmio_setbits_32(MAINPLL_CON0, CON0_RST_BAR);
	mmio_setbits_32(UNIVPLL_CON0, CON0_RST_BAR);

	/* Enable Infra bus divider */
	mmio_setbits_32(INFRA_TOP_DCMCTL, INFRA_TOP_DCM_EN);
	/* Enable Infra DCM */
	mmio_write_32(INFRA_GLOBALCON_DCMDBC,
		      aor(mmio_read_32(INFRA_GLOBALCON_DCMDBC),
			  ~INFRA_GLOBALCON_DCMDBC_MASK,
			  INFRA_GLOBALCON_DCMDBC_ON));
	mmio_write_32(INFRA_GLOBALCON_DCMFSEL,
		      aor(mmio_read_32(INFRA_GLOBALCON_DCMFSEL),
			  ~INFRA_GLOBALCON_DCMFSEL_MASK,
			  INFRA_GLOBALCON_DCMFSEL_ON));
	mmio_write_32(INFRA_GLOBALCON_DCMCTL,
		      aor(mmio_read_32(INFRA_GLOBALCON_DCMCTL),
			  ~INFRA_GLOBALCON_DCMCTL_MASK,
			  INFRA_GLOBALCON_DCMCTL_ON));

	/* Enable Audio external idle */
	mmio_setbits_32(AUDIO_TOP_CON0, AUDIO_TOP_AHB_IDLE_EN);
	/* CPU clock divide by 1 */
	mmio_clrbits_32(INFRA_TOP_CKDIV1, INFRA_TOP_CKDIV1_SEL(0x1f));
	/* Switch to ARM CA7 PLL */
	mmio_setbits_32(INFRA_TOP_CKMUXSEL, INFRA_TOP_MUX_SEL(1));

	/* Set default MUX for topckgen */
	mmio_write_32(CLK_CFG_0,
		      CLK_TOP_AXI_SEL_MUX_SYSPLL1_D2 |
		      CLK_TOP_MEM_SEL_MUX_DMPLL1_CK |
		      CLK_TOP_DDRPHYCFG_SEL_MUX_SYSLL1_D8 |
		      CLK_TOP_ETH_SEL_MUX_UNIVPLL1_D2);
	mmio_write_32(CLK_CFG_1,
		      CLK_TOP_PWM_SEL_MUX_UNIVPLL2_D4 |
		      CLK_TOP_F10M_REF_SEL_MUX_SYSPLL4_D16 |
		      CLK_TOP_NFI_INFRA_SEL_MUX_UNIVPLL2_D8 |
		      CLK_TOP_FLASH_SEL_MUX_SYSPLL2_D8);
	mmio_write_32(CLK_CFG_2,
		      CLK_TOP_UART_SEL_MUX_CLKXTAL |
		      CLK_TOP_SPI0_SEL_MUX_SYSPLL3_D2 |
		      CLK_TOP_SPI1_SEL_MUX_SYSPLL3_D2 |
		      CLK_TOP_MSDC50_0_SEL_MUX_UNIVPLL2_D8);
	mmio_write_32(CLK_CFG_3,
		      CLK_TOP_MSDC30_0_SEL_MUX_UNIV48M |
		      CLK_TOP_MSDC30_1_SEL_MUX_UNIV48M |
		      CLK_TOP_A1SYS_HP_SEL_MUX_AUD1PLL_294P912M |
		      CLK_TOP_A1SYS_HP_SEL_MUX_AUD1PLL_294P912M);
	mmio_write_32(CLK_CFG_4,
		      CLK_TOP_INTDIR_SEL_MUX_UNIVPLL_D2 |
		      CLK_TOP_AUD_INTBUS_SEL_MUX_SYSPLL1_D4 |
		      CLK_TOP_PMICSPI_SEL_MUX_UNIVPLL2_D16 |
		      CLK_TOP_SCP_SEL_MUX_SYSPLL1_D8);
	mmio_write_32(CLK_CFG_5,
		      CLK_TOP_ATB_SEL_MUX_SYSPLL_D5 |
		      CLK_TOP_HIF_SEL_MUX_SYSPLL1_D2 |
		      CLK_TOP_AUDIO_SEL_MUX_SYSPLL3_D4 |
		      CLK_TOP_U2_SEL_MUX_SYSPLL1_D8);
	mmio_write_32(CLK_CFG_6,
		      CLK_TOP_AUD1_SEL_MUX_AUD1PLL_294P912M |
		      CLK_TOP_AUD2_SEL_MUX_AUD2PLL_270P9504M |
		      CLK_TOP_IRRX_SEL_MUX_SYSPLL4_D16 |
		      CLK_TOP_IRTX_SEL_MUX_SYSPLL4_D16);
	mmio_write_32(CLK_CFG_7,
		      CLK_TOP_ASM_L_SEL_MUX_UNIVPLL2_D4 |
		      CLK_TOP_ASM_M_SEL_MUX_UNIVPLL2_D2 |
		      CLK_TOP_ASM_H_SEL_MUX_SYSPLL_D5);
	mmio_write_32(CLK_AUDDIV_0, I2S0_MCK_DIV_PDN | APLL2_MUX_SEL_294M);
	mmio_write_32(CLK_AUDDIV_2, BIT(9) | BIT(25));

	/* Enable scpsys clock off control */
	mmio_write_32(CLK_SCP_CFG_0,
		      SCP_AXICK_DCM_DIS_EN | SCP_AXICK_26M_SEL_EN);
	mmio_write_32(CLK_SCP_CFG_1,
		      SCP_AXICK_DCM_DIS_EN | SCP_AXICK_26M_SEL_EN);

	/* Enable subsystem power domain */
	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_REGWR_CFG_KEY | SPM_REGWR_EN);
	mtcmos_non_cpu_ctrl(1, MT7622_POWER_DOMAIN_ETHSYS);
	mtcmos_non_cpu_ctrl(1, MT7622_POWER_DOMAIN_HIF0);
	mtcmos_non_cpu_ctrl(1, MT7622_POWER_DOMAIN_HIF1);

	/* Clear power-down for clock gates */
	mmio_write_32(INFRA_PDN_CLR0, ~0);
	mmio_write_32(PERI_PDN_CLR0, ~0);
	mmio_write_32(PERI_PDN_CLR1, ~0);

	/** PLL post init **/

	/* Init mempll */
	mtk_mempll_init(DDR1600);

	/* Only UNIVPLL SW Control */
	temp = mmio_read_32(AP_PLL_CON3);
	mmio_write_32(AP_PLL_CON3, temp & 0xFFF44440);

	temp = mmio_read_32(AP_PLL_CON4);
	mmio_write_32(AP_PLL_CON4, temp & 0xFFFFFFF4);
}
