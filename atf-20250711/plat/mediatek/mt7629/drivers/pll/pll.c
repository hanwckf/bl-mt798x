/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <mcucfg.h>
#include <platform_def.h>
#include <spmc.h>
#include "pll.h"

#define aor(v, a, o)			(((v) & (a)) | (o))

void mtk_pll_init(void)
{
	/* Reduce CLKSQ disable time */
	mmio_write_32(CLKSQ_STB_CON0, 0x98940501);

	/* Extend PWR/ISO control timing to 1 us */
	mmio_write_32(PLL_ISO_CON0, BIT(3) | BIT(19));

	mmio_setbits_32(UNIVPLL_CON0, UNIV_48M_EN);

	/* Power on PLL */
	mmio_setbits_32(ARMPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(MAINPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(UNIVPLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(ETH1PLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(ETH2PLL_PWR_CON0, CON0_PWR_ON);
	mmio_setbits_32(SGMIPLL_PWR_CON0, CON0_PWR_ON);

	udelay(1);

	/* Disable PLL ISO */
	mmio_clrbits_32(ARMPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(MAINPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(UNIVPLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(ETH1PLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(ETH2PLL_PWR_CON0, CON0_ISO_EN);
	mmio_clrbits_32(SGMIPLL_PWR_CON0, CON0_ISO_EN);

	/* Set PLL frequency */
	mmio_write_32(ARMPLL_CON1, 0x811e0000);		// 1200MHz
	mmio_write_32(MAINPLL_CON1, 0x811c0000);	// 1120MHz
	mmio_write_32(UNIVPLL_CON1, 0x801e0000);	// 1200MHz
	mmio_write_32(ETH1PLL_CON1, 0x80190000);	// 500MHz
	mmio_write_32(ETH2PLL_CON1, 0x80118000);	// 750MHz
	mmio_write_32(SGMIPLL_CON1, 0x80104000);	// 650MHz

	/* Enable PLL frequency */
	mmio_setbits_32(ARMPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(MAINPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(UNIVPLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(ETH1PLL_CON0, CON0_BASE_EN);
	mmio_setbits_32(ETH2PLL_CON0, CON0_BASE_EN);
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

	/* Change CPU:CCI clock ratio to 1:2 */
	mmio_clrsetbits_32(MCU_AXI_DIV, AXI_DIV_MSK, AXI_DIV_SEL(0x12));
	/* Switch to ARM CA7 PLL */
	mmio_clrsetbits_32(MCU_BUS_MUX, MCU_BUS_MSK, MCU_BUS_SEL(0x1));

	/* Set default MUX for topckgen */
	mmio_write_32(CLK_CFG_0,
		      CLK_TOP_AXI_SEL_MUX_SYSPLL1_D2 |
		      CLK_TOP_DDRPHYCFG_SEL_MUX_SYSLL1_D8 |
		      CLK_TOP_ETH_SEL_MUX_UNIVPLL1_D2);
	mmio_write_32(CLK_CFG_1,
		      CLK_TOP_PWM_SEL_MUX_UNIVPLL2_D4 |
		      CLK_TOP_F10M_REF_SEL_MUX_SYSPLL4_D16 |
		      CLK_TOP_NFI_INFRA_SEL_MUX_XTAL_CLK);
	mmio_write_32(CLK_CFG_2,
		      CLK_TOP_UART_SEL_MUX_CLKXTAL |
		      CLK_TOP_SPI0_SEL_MUX_SYSPLL3_D2 |
		      CLK_TOP_SPI1_SEL_MUX_SYSPLL3_D2 |
		      CLK_TOP_MSDC50_0_SEL_MUX_UNIVPLL2_D8);
	mmio_write_32(CLK_CFG_3,
		      CLK_TOP_MSDC30_0_SEL_MUX_UNIV48M |
		      CLK_TOP_MSDC30_1_SEL_MUX_UNIV200M |
		      CLK_TOP_A1SYS_HP_SEL_MUX_XTAL |
		      CLK_TOP_A2SYS_HP_SEL_MUX_XTAL);
	mmio_write_32(CLK_CFG_4,
		      CLK_TOP_INTDIR_SEL_MUX_UNIVPLL_D2 |
		      CLK_TOP_AUD_INTBUS_SEL_MUX_SYSPLL1_D4 |
		      CLK_TOP_PMICSPI_SEL_MUX_UNIVPLL2_D4 |
		      CLK_TOP_SCP_SEL_MUX_SYSPLL1_D8);
	mmio_write_32(CLK_CFG_5,
		      CLK_TOP_ATB_SEL_MUX_SYSPLL_D2 |
		      CLK_TOP_HIF_SEL_MUX_SYSPLL1_D2 |
		      CLK_TOP_AUDIO_SEL_MUX_SYSPLL3_D4 |
		      CLK_TOP_U2_SEL_MUX_SYSPLL1_D4);
	mmio_write_32(CLK_CFG_6,
		      CLK_TOP_IRRX_SEL_MUX_SYSPLL4_D16 |
		      CLK_TOP_IRTX_SEL_MUX_SYSPLL4_D16);
	mmio_write_32(CLK_CFG_7,
		      CLK_TOP_ASM_L_SEL_MUX_SYSPLL_D5 |
		      CLK_TOP_ASM_M_SEL_MUX_SYSPLL_D5 |
		      CLK_TOP_ASM_H_SEL_MUX_SYSPLL_D5 |
		      BIT(24));
	mmio_write_32(CLK_CFG_8,
		      CLK_TOP_CRYPTO_SEL_MUX_UNIVPLL_D3 |
		      CLK_TOP_SGMII_SEL_MUX_SGMIIPLL_D3 |
		      CLK_TOP_10M_SEL_MUX_XTAL_D4);

	/* Enable scpsys clock off control */
	mmio_write_32(CLK_SCP_CFG_0, SCP_ARMCK_OFF_EN);
	mmio_write_32(CLK_SCP_CFG_1,
		      SCP_AXICK_DCM_DIS_EN | SCP_AXICK_26M_SEL_EN);

	/* Enable subsystem power domain */
	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_REGWR_CFG_KEY | SPM_REGWR_EN);
	mtk_spm_non_cpu_ctrl(1, MT7629_POWER_DOMAIN_ETHSYS);
	mtk_spm_non_cpu_ctrl(1, MT7629_POWER_DOMAIN_HIF0);
	mtk_spm_non_cpu_ctrl(1, MT7629_POWER_DOMAIN_HIF1);

	/* Clear power-down for clock gates */
	mmio_write_32(INFRA_PDN_CLR0, ~0);
	mmio_write_32(PERI_PDN_CLR0, ~0);
	mmio_write_32(PERI_PDN_CLR1, 0x7);
}
