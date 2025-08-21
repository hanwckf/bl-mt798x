// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 * Author: SkyLake.Huang <skylake.huang@mediatek.com>
 */

#include <errno.h>
#include <drivers/delay_timer.h>
#include "mtk-phy-lib.h"

#define MD32_EN					BIT(0)

#define MTK_2P5GPHY_ID_MT7987			(0x00339c91)
#define MTK_2P5GPHY_ID_MT7988			(0x00339c11)

#define MTK_2P5GPHY_PMD_REG_BASE		(0x0f010000)
#define MTK_2P5GPHY_PMD_REG_LEN			(0x210)
#define DO_NOT_RESET				(0x28)
#define   DO_NOT_RESET_XBZ			BIT(0)
#define   DO_NOT_RESET_PMA			BIT(3)
#define   DO_NOT_RESET_RX			BIT(5)
#define FNPLL_PWR_CTRL1				(0x208)
#define   RG_SPEED_MASK				GENMASK(3, 0)
#define   RG_SPEED_2500				BIT(3)
#define   RG_SPEED_100				BIT(0)
#define FNPLL_PWR_CTRL_STATUS			(0x20c)
#define   RG_STABLE_MASK			GENMASK(3, 0)
#define   RG_SPEED_2500_STABLE			BIT(3)
#define   RG_SPEED_100_STABLE			BIT(0)

#define MTK_2P5GPHY_XBZ_PCS_REG_BASE		(0x0f030000)
#define MTK_2P5GPHY_XBZ_PCS_REG_LEN		(0x844)
#define PHY_CTRL_CONFIG				(0x200)
#define PMU_WP					(0x800)
#define   WRITE_PROTECT_KEY			(0xCAFEF00D)
#define PMU_PMA_AUTO_CFG			(0x820)
#define   POWER_ON_AUTO_MODE			BIT(16)
#define   PMU_AUTO_MODE_EN			BIT(0)
#define PMU_PMA_STATUS				(0x840)
#define   CLK_IS_DISABLED			BIT(3)

#define MTK_2P5GPHY_XBZ_PMA_RX_BASE		(0x0f080000)
#define MTK_2P5GPHY_XBZ_PMA_RX_LEN		(0x5228)
#define SMEM_WDAT0				(0x5000)
#define SMEM_WDAT1				(0x5004)
#define SMEM_WDAT2				(0x5008)
#define SMEM_WDAT3				(0x500c)
#define SMEM_CTRL				(0x5024)
#define   SMEM_HW_RDATA_ZERO			BIT(24)
#define SMEM_ADDR_REF_ADDR			(0x502c)
#define CM_CTRL_P01				(0x5100)
#define CM_CTRL_P23				(0x5124)
#define DM_CTRL_P01				(0x5200)
#define DM_CTRL_P23				(0x5224)

#define MTK_2P5GPHY_CHIP_SCU_BASE		(0x0f0cf800)
#define MTK_2P5GPHY_CHIP_SCU_LEN		(0x12c)
#define SYS_SW_RESET				(0x128)
#define   RESET_RST_CNT				BIT(0)

#define MTK_2P5GPHY_MCU_CSR_BASE		(0x0f0f0000)
#define MTK_2P5GPHY_MCU_CSR_LEN			(0x20)
#define MD32_EN_CFG				(0x18)
#define   MD32_EN				BIT(0)

#define MTK_2P5GPHY_PMB_FW_BASE			(0x0f100000)

#define MTK_2P5GPHY_APB_BASE			(0x11c30000)
#define MTK_2P5GPHY_APB_LEN			(0x9c)
#define SW_RESET				(0x94)
#define   MD32_RESTART_EN_CLEAR			BIT(9)

#define BASE100T_STATUS_EXTEND			(0x10)
#define BASE1000T_STATUS_EXTEND			(0x11)
#define EXTEND_CTRL_AND_STATUS			(0x16)

#define PHY_AUX_CTRL_STATUS			(0x1d)
#define   PHY_AUX_DPX_MASK			GENMASK(5, 5)
#define   PHY_AUX_SPEED_MASK			GENMASK(4, 2)

/* Registers on MDIO_MMD_VEND1 */
#define MTK_PHY_LINK_STATUS_RELATED		(0x147)
#define   MTK_PHY_BYPASS_LINK_STATUS_OK		BIT(4)
#define   MTK_PHY_FORCE_LINK_STATUS_HCD		BIT(3)

#define MTK_PHY_AN_FORCE_SPEED_REG		(0x313)
#define   MTK_PHY_MASTER_FORCE_SPEED_SEL_EN	BIT(7)
#define   MTK_PHY_MASTER_FORCE_SPEED_SEL_MASK	GENMASK(6, 0)

#define MTK_PHY_LPI_PCS_DSP_CTRL		(0x121)
#define   MTK_PHY_LPI_SIG_EN_LO_THRESH100_MASK	GENMASK(12, 8)

/* Registers on Token Ring debug nodes */
/* ch_addr = 0x0, node_addr = 0xf, data_addr = 0x3c */
#define AUTO_NP_10XEN				BIT(6)

#include "plat_i2p5ge_init.h"

int eth_phy_init(uint8_t phy)
{
	i2p5ge_phy_plat_init(phy);

	/* Setup LED */
	mtk_mdio_set_bits(phy, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
			  MTK_PHY_LED_ON_LINK10 |
			  MTK_PHY_LED_ON_LINK100 |
			  MTK_PHY_LED_ON_LINK1000 |
			  MTK_PHY_LED_ON_LINK2500);
	mtk_mdio_set_bits(phy, MDIO_MMD_VEND2, MTK_PHY_LED1_ON_CTRL,
			  MTK_PHY_LED_ON_FDX | MTK_PHY_LED_ON_HDX);

	mtk_mdio_clrset_bits(phy, MDIO_MMD_VEND1, MTK_PHY_LPI_PCS_DSP_CTRL,
			     MTK_PHY_LPI_SIG_EN_LO_THRESH100_MASK, 0);

	/* Enable 16-bit next page exchange bit if 1000-BT isn't advertising */
	mtk_tr_modify(phy, 0x0, 0xf, 0x3c, AUTO_NP_10XEN,
		      FIELD_PREP(AUTO_NP_10XEN, 0x1));

	/* Enable HW auto downshift */
	mtk_phy_select_page(phy, MTK_PHY_PAGE_EXTENDED_1);
	mtk_mdio_clrset_bits(phy, MDIO_MMD_VEND1, MTK_PHY_AUX_CTRL_AND_STATUS,
			     0, MTK_PHY_ENABLE_DOWNSHIFT);
	mtk_phy_restore_page(phy);

	return 0;
}

int eth_phy_wait_ready(uint8_t phy, uint32_t timeout_ms)
{
	if (!timeout_ms)
		timeout_ms = UINT32_MAX;

	return gen_phy_wait_ready(phy, timeout_ms);
}

int eth_phy_start(uint8_t phy)
{
	return 0;
}

int eth_phy_get_status(uint8_t phy, struct genphy_status *status)
{
	int ret;

	ret = eth_phy_wait_ready(phy, MTK_ETH_AUTONEG_TIMEOUT * 1000);
	if (ret) {
		ERROR("PHY: Timeout waiting for port link up\n");
		return ret;
	}

	gen_phy_get_status(phy, status);

	return 0;
}
