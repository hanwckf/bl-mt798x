// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2023 Freescale Semiconductor, Inc.
 * author SkyLake.Huang
 */
#include <asm/io.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <dm/of_access.h>
#include <dm/ofnode.h>
#include <dm/pinctrl.h>
#include <linux/bitfield.h>
#include <linux/delay.h>
#include <linux/iopoll.h>
#include <phy.h>
#include "mt7988_i2p5ge_phy_pmb.h"
#include "mt7987_i2p5ge_phy_pmb.h"
#include "mt7987_i2p5ge_phy_DSPBitTb.h"
#include "mtk.h"

#define MD32_EN			BIT(0)

#define MTK_2P5GPHY_ID_MT7987	(0x00339c91)
#define MTK_2P5GPHY_ID_MT7988	(0x00339c11)

#define MT7987_2P5GE_PMB_FW_SIZE	(0x18000)
#define MT7987_2P5GE_DSPBITTB_SIZE	(0x7000)

#define MT7988_2P5GE_PMB_FW_SIZE	(0x20000)
#define MT7988_2P5GE_PMB_FW_BASE	(0x0f100000)
#define MT7988_2P5GE_PMB_FW_LEN		(0x20000)

#define MTK_2P5GPHY_PMD_REG_BASE	(0x0f010000)
#define MTK_2P5GPHY_PMD_REG_LEN		(0x210)
#define DO_NOT_RESET			(0x28)
#define   DO_NOT_RESET_XBZ		BIT(0)
#define   DO_NOT_RESET_PMA		BIT(3)
#define   DO_NOT_RESET_RX		BIT(5)
#define FNPLL_PWR_CTRL1			(0x208)
#define   RG_SPEED_MASK			GENMASK(3, 0)
#define   RG_SPEED_2500			BIT(3)
#define   RG_SPEED_100			BIT(0)
#define FNPLL_PWR_CTRL_STATUS		(0x20c)
#define   RG_STABLE_MASK		GENMASK(3, 0)
#define   RG_SPEED_2500_STABLE		BIT(3)
#define   RG_SPEED_100_STABLE		BIT(0)

#define MTK_2P5GPHY_XBZ_PCS_REG_BASE	(0x0f030000)
#define MTK_2P5GPHY_XBZ_PCS_REG_LEN	(0x844)
#define PHY_CTRL_CONFIG			(0x200)
#define PMU_WP				(0x800)
#define   WRITE_PROTECT_KEY		(0xCAFEF00D)
#define PMU_PMA_AUTO_CFG		(0x820)
#define   POWER_ON_AUTO_MODE		BIT(16)
#define   PMU_AUTO_MODE_EN		BIT(0)
#define PMU_PMA_STATUS			(0x840)
#define   CLK_IS_DISABLED		BIT(3)

#define MTK_2P5GPHY_XBZ_PMA_RX_BASE	(0x0f080000)
#define MTK_2P5GPHY_XBZ_PMA_RX_LEN	(0x5228)
#define SMEM_WDAT0			(0x5000)
#define SMEM_WDAT1			(0x5004)
#define SMEM_WDAT2			(0x5008)
#define SMEM_WDAT3			(0x500c)
#define SMEM_CTRL			(0x5024)
#define   SMEM_HW_RDATA_ZERO		BIT(24)
#define SMEM_ADDR_REF_ADDR		(0x502c)
#define CM_CTRL_P01			(0x5100)
#define CM_CTRL_P23			(0x5124)
#define DM_CTRL_P01			(0x5200)
#define DM_CTRL_P23			(0x5224)

#define MTK_2P5GPHY_CHIP_SCU_BASE	(0x0f0cf800)
#define MTK_2P5GPHY_CHIP_SCU_LEN	(0x12c)
#define SYS_SW_RESET			(0x128)
#define   RESET_RST_CNT			BIT(0)

#define MTK_2P5GPHY_MCU_CSR_BASE	(0x0f0f0000)
#define MTK_2P5GPHY_MCU_CSR_LEN		(0x20)
#define MD32_EN_CFG			(0x18)
#define   MD32_EN			BIT(0)

#define MTK_2P5GPHY_PMB_FW_BASE		(0x0f100000)

#define MTK_2P5GPHY_APB_BASE		(0x11c30000)
#define MTK_2P5GPHY_APB_LEN		(0x9c)
#define SW_RESET			(0x94)
#define   MD32_RESTART_EN_CLEAR		BIT(9)

#define BASE100T_STATUS_EXTEND		(0x10)
#define BASE1000T_STATUS_EXTEND		(0x11)
#define EXTEND_CTRL_AND_STATUS		(0x16)

#define PHY_AUX_CTRL_STATUS		(0x1d)
#define   PHY_AUX_DPX_MASK		GENMASK(5, 5)
#define   PHY_AUX_SPEED_MASK		GENMASK(4, 2)

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

struct mtk_i2p5ge_phy_priv {
	bool fw_loaded;
};

static int mt7987_2p5ge_phy_load_fw(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	struct udevice *dev = phydev->dev;
	void __iomem *xbz_pcs_reg_base;
	void __iomem *xbz_pma_rx_base;
	void __iomem *chip_scu_base;
	void __iomem *pmd_reg_base;
	void __iomem *mcu_csr_base;
	void __iomem *apb_base;
	void __iomem *pmb_addr;
	size_t fw_size = 0;
	u8 *fw;
	int ret, i;
	u32 reg;

	if (priv->fw_loaded)
		return 0;

	apb_base = map_physmem(MTK_2P5GPHY_APB_BASE,
			       MTK_2P5GPHY_APB_LEN, MAP_NOCACHE);
	if (!apb_base)
		return -ENOMEM;

	pmd_reg_base = map_physmem(MTK_2P5GPHY_PMD_REG_BASE,
				   MTK_2P5GPHY_PMD_REG_LEN, MAP_NOCACHE);
	if (!pmd_reg_base) {
		ret = -ENOMEM;
		goto free_apb;
	}

	xbz_pcs_reg_base = map_physmem(MTK_2P5GPHY_XBZ_PCS_REG_BASE,
				       MTK_2P5GPHY_XBZ_PCS_REG_LEN,
				       MAP_NOCACHE);
	if (!xbz_pcs_reg_base) {
		ret = -ENOMEM;
		goto free_pmd;
	}

	xbz_pma_rx_base = map_physmem(MTK_2P5GPHY_XBZ_PMA_RX_BASE,
			              MTK_2P5GPHY_XBZ_PMA_RX_LEN, MAP_NOCACHE);
	if (!xbz_pma_rx_base) {
		ret = -ENOMEM;
		goto free_pcs;
	}

	chip_scu_base = map_physmem(MTK_2P5GPHY_CHIP_SCU_BASE,
				    MTK_2P5GPHY_CHIP_SCU_LEN, MAP_NOCACHE);
	if (!chip_scu_base) {
		ret = -ENOMEM;
		goto free_pma;
	}

	mcu_csr_base = map_physmem(MTK_2P5GPHY_MCU_CSR_BASE,
				   MTK_2P5GPHY_MCU_CSR_LEN, MAP_NOCACHE);
	if (!mcu_csr_base) {
		ret = -ENOMEM;
		goto free_chip_scu;
	}

	pmb_addr = map_physmem(MTK_2P5GPHY_PMB_FW_BASE,
			       MT7987_2P5GE_PMB_FW_SIZE, MAP_NOCACHE);
	if (!pmb_addr) {
		return -ENOMEM;
		goto free_mcu_csr;
	}

	fw = (u8 *)mt7987_i2p5ge_phy_pmb;
	fw_size = sizeof(mt7987_i2p5ge_phy_pmb);
	if (fw_size != MT7987_2P5GE_PMB_FW_SIZE) {
		dev_err(dev, "PMb firmware size 0x%zx != 0x%x\n",
			fw_size, MT7987_2P5GE_PMB_FW_SIZE);
		ret = -EINVAL;
		goto free_pmb_addr;
	}

	/* Force 2.5Gphy back to AN state */
	phy_set_bits_mmd(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);
	udelay(5000);
	phy_set_bits_mmd(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_PDOWN);

	reg = readw(apb_base + SW_RESET);
	writew(reg & ~MD32_RESTART_EN_CLEAR, apb_base + SW_RESET);
	writew(reg | MD32_RESTART_EN_CLEAR, apb_base + SW_RESET);
	writew(reg & ~MD32_RESTART_EN_CLEAR, apb_base + SW_RESET);

	reg = readw(mcu_csr_base + MD32_EN_CFG);
	writew(reg & ~MD32_EN, mcu_csr_base + MD32_EN_CFG);

	for(i = 0; i < (MT7987_2P5GE_PMB_FW_SIZE >> 2); i++) {
		writel(*((u32 *)fw + i), (u32 *)pmb_addr + i);
	}
	dev_info(phydev->dev, "Firmware date code: %x/%x/%x, version: %x.%x\n",
		 be16_to_cpu(*((__be16 *)(fw + MT7987_2P5GE_PMB_FW_SIZE - 8))),
		 *(fw + MT7987_2P5GE_PMB_FW_SIZE - 6),
		 *(fw + MT7987_2P5GE_PMB_FW_SIZE - 5),
		 *(fw + MT7987_2P5GE_PMB_FW_SIZE - 2),
		 *(fw + MT7987_2P5GE_PMB_FW_SIZE - 1));

	/* Enable 100Mbps module clock. */
	writel(FIELD_PREP(RG_SPEED_MASK, RG_SPEED_100),
	       pmd_reg_base + FNPLL_PWR_CTRL1);

	/* Check if 100Mbps module clock is ready. */
	ret = readl_poll_timeout(pmd_reg_base + FNPLL_PWR_CTRL_STATUS, reg,
				 reg & RG_SPEED_100_STABLE, 10000);
	if (ret)
		dev_err(dev, "Fail to enable 100Mbps module clock: %d\n", ret);

	/* Enable 2.5Gbps module clock. */
	writel(FIELD_PREP(RG_SPEED_MASK, RG_SPEED_2500),
	       pmd_reg_base + FNPLL_PWR_CTRL1);

	/* Check if 2.5Gbps module clock is ready. */
	ret = readl_poll_timeout(pmd_reg_base + FNPLL_PWR_CTRL_STATUS, reg,
				 reg & RG_SPEED_2500_STABLE, 10000);

	if (ret)
		dev_err(dev, "Fail to enable 2.5Gbps module clock: %d\n", ret);

	/* Disable AN */
	phy_clear_bits_mmd(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_ANENABLE);

	/* Force to run at 2.5G speed */
	phy_modify_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_AN_FORCE_SPEED_REG,
		       MTK_PHY_MASTER_FORCE_SPEED_SEL_MASK,
		       MTK_PHY_MASTER_FORCE_SPEED_SEL_EN |
		       FIELD_PREP(MTK_PHY_MASTER_FORCE_SPEED_SEL_MASK, 0x1b));

	phy_set_bits_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_LINK_STATUS_RELATED,
			 MTK_PHY_BYPASS_LINK_STATUS_OK |
			 MTK_PHY_FORCE_LINK_STATUS_HCD);

	/* Set xbz, pma and rx as "do not reset" in order to input DSP code. */
	reg = readl(pmd_reg_base + DO_NOT_RESET);
	reg |= DO_NOT_RESET_XBZ | DO_NOT_RESET_PMA | DO_NOT_RESET_RX;
	writel(reg, pmd_reg_base + DO_NOT_RESET);

	reg = readl(chip_scu_base + SYS_SW_RESET);
	writel(reg & ~RESET_RST_CNT, chip_scu_base + SYS_SW_RESET);

	writel(WRITE_PROTECT_KEY, xbz_pcs_reg_base + PMU_WP);

	reg = readl(xbz_pcs_reg_base + PMU_PMA_AUTO_CFG);
	reg |= PMU_AUTO_MODE_EN | POWER_ON_AUTO_MODE;
	writel(reg, xbz_pcs_reg_base + PMU_PMA_AUTO_CFG);

	/* Check if clock in auto mode is disabled. */
	ret = readl_poll_timeout(xbz_pcs_reg_base + PMU_PMA_STATUS, reg,
				 (reg & CLK_IS_DISABLED) == 0x0, 100000);
	if (ret)
		dev_err(dev, "Clock isn't disabled in auto mode: %d\n", ret);

	reg = readl(xbz_pma_rx_base + SMEM_CTRL);
	writel(reg | SMEM_HW_RDATA_ZERO, xbz_pma_rx_base + SMEM_CTRL);

	reg = readl(xbz_pcs_reg_base + PHY_CTRL_CONFIG);
	writel(reg | BIT(16), xbz_pcs_reg_base + PHY_CTRL_CONFIG);

	/* Initialize data memory */
	reg = readl(xbz_pma_rx_base + DM_CTRL_P01);
	writel(reg | BIT(28), xbz_pma_rx_base + DM_CTRL_P01);
	reg = readl(xbz_pma_rx_base + DM_CTRL_P23);
	writel(reg | BIT(28), xbz_pma_rx_base + DM_CTRL_P23);

	/* Initialize coefficient memory */
	reg = readl(xbz_pma_rx_base + CM_CTRL_P01);
	writel(reg | BIT(28), xbz_pma_rx_base + CM_CTRL_P01);
	reg = readl(xbz_pma_rx_base + CM_CTRL_P23);
	writel(reg | BIT(28), xbz_pma_rx_base + CM_CTRL_P23);

	/* Initilize PM offset */
	writel(0, xbz_pma_rx_base + SMEM_ADDR_REF_ADDR);

	fw = (u8 *)mt7987_i2p5ge_phy_DSPBitTb;
	fw_size = sizeof(mt7987_i2p5ge_phy_DSPBitTb);
	if (fw_size != MT7987_2P5GE_DSPBITTB_SIZE) {
		dev_err(dev, "PMb firmware size 0x%zx != 0x%x\n",
			fw_size, MT7987_2P5GE_DSPBITTB_SIZE);
		ret = -EINVAL;
		goto free_pmb_addr;
	}

	for (i = 0; i < MT7987_2P5GE_DSPBITTB_SIZE - 1; i += 16) {
		writel(*((uint32_t *)(fw + i)),
		       xbz_pma_rx_base + SMEM_WDAT0);
		writel(*((uint32_t *)(fw + i + 0x4)),
		       xbz_pma_rx_base + SMEM_WDAT1);
		writel(*((uint32_t *)(fw + i + 0x8)),
		       xbz_pma_rx_base + SMEM_WDAT2);
		writel(*((uint32_t *)(fw + i + 0xc)),
		       xbz_pma_rx_base + SMEM_WDAT3);
	}

	reg = readl(xbz_pma_rx_base + DM_CTRL_P01);
	writel(reg & ~BIT(28), xbz_pma_rx_base + DM_CTRL_P01);

	reg = readl(xbz_pma_rx_base + DM_CTRL_P23);
	writel(reg & ~BIT(28), xbz_pma_rx_base + DM_CTRL_P23);

	reg = readl(xbz_pma_rx_base + CM_CTRL_P01);
	writel(reg & ~BIT(28), xbz_pma_rx_base + CM_CTRL_P01);

	reg = readl(xbz_pma_rx_base + CM_CTRL_P23);
	writel(reg & ~BIT(28), xbz_pma_rx_base + CM_CTRL_P23);

	reg = readw(mcu_csr_base + MD32_EN_CFG);
	writew(reg | MD32_EN, mcu_csr_base + MD32_EN_CFG);
	phy_set_bits_mmd(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);
	/* We need a delay here to stabilize initialization of MCU */
	udelay(8000);
	dev_info(dev, "Firmware loading/trigger ok.\n");

	priv->fw_loaded = true;

free_pmb_addr:
	unmap_physmem(pmb_addr, MAP_NOCACHE);
free_mcu_csr:
	unmap_physmem(mcu_csr_base, MAP_NOCACHE);
free_chip_scu:
	unmap_physmem(chip_scu_base, MAP_NOCACHE);
free_pma:
	unmap_physmem(xbz_pma_rx_base, MAP_NOCACHE);
free_pcs:
	unmap_physmem(xbz_pcs_reg_base, MAP_NOCACHE);
free_pmd:
	unmap_physmem(pmd_reg_base, MAP_NOCACHE);
free_apb:
	unmap_physmem(apb_base, MAP_NOCACHE);

	return ret;
}

static int mt7988_2p5ge_phy_load_fw(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv = phydev->priv;
	void __iomem *mcu_csr_base, *pmb_addr;
	size_t fw_size = 0;
	u8 *fw;
	int ret, i;
	u16 reg;

	if (priv->fw_loaded)
		return 0;

	pmb_addr = map_physmem(MT7988_2P5GE_PMB_FW_BASE,
			       MT7988_2P5GE_PMB_FW_LEN, MAP_NOCACHE);
	if (!pmb_addr)
		return -ENOMEM;
	mcu_csr_base = map_physmem(MTK_2P5GPHY_MCU_CSR_BASE,
				   MTK_2P5GPHY_MCU_CSR_LEN, MAP_NOCACHE);
	if (!mcu_csr_base) {
		ret = -ENOMEM;
		goto free_pmb;
	}

	fw = (u8 *)mt7988_i2p5ge_phy_pmb;
	fw_size = sizeof(mt7988_i2p5ge_phy_pmb);
	if (fw_size != MT7988_2P5GE_PMB_FW_SIZE) {
		dev_err(phydev->dev, "Firmware size 0x%zx != 0x%x\n",
			fw_size, MT7988_2P5GE_PMB_FW_SIZE);
		ret = -EINVAL;
		goto free;
	}
	/* Firmware is arranged in 4 byte-aligned */
	for(i = 0; i < (MT7988_2P5GE_PMB_FW_SIZE >> 2); i++) {
		writel(*((u32 *)fw + i), (u32 *)pmb_addr + i);
	}
	dev_info(phydev->dev, "Firmware date code: %x/%x/%x, version: %x.%x\n",
		 be16_to_cpu(*((__be16 *)(fw + MT7988_2P5GE_PMB_FW_SIZE - 8))),
		 *(fw + MT7988_2P5GE_PMB_FW_SIZE - 6),
		 *(fw + MT7988_2P5GE_PMB_FW_SIZE - 5),
		 *(fw + MT7988_2P5GE_PMB_FW_SIZE - 2),
		 *(fw + MT7988_2P5GE_PMB_FW_SIZE - 1));

	reg = readw(mcu_csr_base + MD32_EN_CFG);
	writew(reg & ~MD32_EN, mcu_csr_base + MD32_EN_CFG);
	writew(reg | MD32_EN, mcu_csr_base + MD32_EN_CFG);
	phy_set_bits_mmd(phydev, MDIO_DEVAD_NONE, MII_BMCR, BMCR_RESET);
	/* We need a delay here to stabilize initialization of MCU */
	udelay(8000);
	dev_info(phydev->dev, "Firmware loading/trigger ok.\n");

	priv->fw_loaded = true;

free:
	unmap_physmem(mcu_csr_base, MAP_NOCACHE);
free_pmb:
	unmap_physmem(pmb_addr, MAP_NOCACHE);

	return ret;
}

static int mt798x_2p5ge_phy_config_init(struct phy_device *phydev)
{
	int ret;

	/* Check if PHY interface type is compatible */
	/*if (phydev->interface != PHY_INTERFACE_MODE_INTERNAL)
		return -ENODEV;*/

	switch (phydev->phy_id) {
	case MTK_2P5GPHY_ID_MT7987:
		ret = mt7987_2p5ge_phy_load_fw(phydev);
		phy_clear_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
				   MTK_PHY_LED_ON_POLARITY);
		break;
	case MTK_2P5GPHY_ID_MT7988:
		ret = mt7988_2p5ge_phy_load_fw(phydev);
		phy_set_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
				 MTK_PHY_LED_ON_POLARITY);
		break;
	default:
		return -EINVAL;
	}
	if (ret < 0)
		return ret;
	/* Setup LED */
	phy_set_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
			 MTK_PHY_LED_ON_LINK10 |
			 MTK_PHY_LED_ON_LINK100 |
			 MTK_PHY_LED_ON_LINK1000 |
			 MTK_PHY_LED_ON_LINK2500);
	phy_set_bits_mmd(phydev, MDIO_MMD_VEND2, MTK_PHY_LED1_ON_CTRL,
			 MTK_PHY_LED_ON_FDX | MTK_PHY_LED_ON_HDX);

	/* Switch pinctrl after setting polarity to avoid bogus blinking */
	ret = pinctrl_select_state(phydev->dev, "i2p5gbe-led");
	if (ret)
		dev_err(phydev->dev, "Fail to set LED pins!\n");

	phy_modify_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_LPI_PCS_DSP_CTRL,
		       MTK_PHY_LPI_SIG_EN_LO_THRESH100_MASK, 0);

	/* Enable 16-bit next page exchange bit if 1000-BT isn't advertising */
	mtk_tr_modify(phydev, 0x0, 0xf, 0x3c, AUTO_NP_10XEN,
		      FIELD_PREP(AUTO_NP_10XEN, 0x1));

	/* Enable HW auto downshift */
	mtk_phy_select_page(phydev, MTK_PHY_PAGE_EXTENDED_1);
	phy_modify_mmd(phydev, MDIO_MMD_VEND1, MTK_PHY_AUX_CTRL_AND_STATUS,
		       0, MTK_PHY_ENABLE_DOWNSHIFT);
	mtk_phy_restore_page(phydev);

	return 0;
}

static int mt798x_2p5ge_phy_probe(struct phy_device *phydev)
{
	struct mtk_i2p5ge_phy_priv *priv;

	priv = devm_kzalloc(phydev->dev,
			    sizeof(struct mtk_i2p5ge_phy_priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->fw_loaded = false;
	phydev->priv = priv;

	return 0;
}

U_BOOT_PHY_DRIVER(mt7988_i2p5g) = {
	.name = "Mediatek MT7988 internal 2.5GPHY",
	.uid = MTK_2P5GPHY_ID_MT7988,
	.mask = 0xfffffff0,
	.features = PHY_10G_FEATURES,
	.mmds = (MDIO_MMD_PCS | MDIO_MMD_AN |
		 MDIO_MMD_VEND1 | MDIO_MMD_VEND2),
	.probe = &mt798x_2p5ge_phy_probe,
	.config = &mt798x_2p5ge_phy_config_init,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};

U_BOOT_PHY_DRIVER(mt7987_i2p5g) = {
	.name = "Mediatek MT7987 internal 2.5GPHY",
	.uid = MTK_2P5GPHY_ID_MT7987,
	.mask = 0xfffffff0,
	.features = PHY_10G_FEATURES,
	.mmds = (MDIO_MMD_PCS | MDIO_MMD_AN |
		 MDIO_MMD_VEND1 | MDIO_MMD_VEND2),
	.probe = &mt798x_2p5ge_phy_probe,
	.config = &mt798x_2p5ge_phy_config_init,
	.startup = &genphy_startup,
	.shutdown = &genphy_shutdown,
};
