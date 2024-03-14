/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <common/debug.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat_eth_def.h>
#include <pll.h>
#include "mtk_eth_internal.h"

#define NUM_TX_DESC		8
#define NUM_RX_DESC		8
#define PKTSIZE_ALIGN		1536
#define ARCH_DMA_MINALIGN	64
#define TX_TOTAL_BUF_SIZE	(NUM_TX_DESC * PKTSIZE_ALIGN)
#define RX_TOTAL_BUF_SIZE	(NUM_RX_DESC * PKTSIZE_ALIGN)
#define PKT_TOTAL_BUF_SIZE	(TX_TOTAL_BUF_SIZE + RX_TOTAL_BUF_SIZE)
#define TX_RX_RING_SIZE		PAGE_SIZE
#define ALIGNED_PKT_BUF_SIZE	round_up(PKT_TOTAL_BUF_SIZE, PAGE_SIZE)

#define MT753X_NUM_PHYS		5
#define MT753X_NUM_PORTS	7
#define MT753X_DFL_SMI_ADDR	31
#define MT753X_SMI_ADDR_MASK	0x1f

#define MT753X_PHY_ADDR(base, addr) \
	(((base) + (addr)) & 0x1f)

#define GDMA_FWD_TO_CPU \
	(0x20000000 | \
	GDM_ICS_EN | \
	GDM_TCS_EN | \
	GDM_UCS_EN | \
	STRP_CRC | \
	(DP_PDMA << MYMAC_DP_S) | \
	(DP_PDMA << BC_DP_S) | \
	(DP_PDMA << MC_DP_S) | \
	(DP_PDMA << UN_DP_S))

#define GDMA_BRIDGE_TO_CPU \
	(0xC0000000 | \
	GDM_ICS_EN | \
	GDM_TCS_EN | \
	GDM_UCS_EN | \
	(DP_PDMA << MYMAC_DP_S) | \
	(DP_PDMA << BC_DP_S) | \
	(DP_PDMA << MC_DP_S) | \
	(DP_PDMA << UN_DP_S))

#define GDMA_FWD_DISCARD \
	(0x20000000 | \
	GDM_ICS_EN | \
	GDM_TCS_EN | \
	GDM_UCS_EN | \
	STRP_CRC | \
	(DP_DISCARD << MYMAC_DP_S) | \
	(DP_DISCARD << BC_DP_S) | \
	(DP_DISCARD << MC_DP_S) | \
	(DP_DISCARD << UN_DP_S))

struct mtk_eth_priv {
	uintptr_t pkt_pool_pa;
	uintptr_t pkt_pool_va;

	uintptr_t tx_ring_pa;
	uintptr_t rx_ring_pa;
	void *tx_ring_noc;
	void *rx_ring_noc;

	int rx_dma_owner_idx0;
	int tx_cpu_owner_idx0;

	uint32_t mt753x_phy_base;
	uint32_t mt753x_pmcr;
};

#if GMAC_FORCE_MODE && (GMAC_FORCE_SPEED != SPEED_1000) && \
    (GMAC_FORCE_SPEED != SPEED_2500) && (GMAC_FORCE_SPEED != SPEED_10000)
#error Invalid speed set for fixed-link
#endif

#if defined(SWITCH_MT7530)
#define SWITCH_RESET_WAIT_MS	1000
#define mt753x_mii_read		mtk_mii_read
#define mt753x_mii_write	mtk_mii_write
#define mt753x_mmd_read		mtk_mmd_ind_read
#define mt753x_mmd_write	mtk_mmd_ind_write
static int mtk_mmd_ind_read(struct mtk_eth_priv *priv, uint8_t addr,
			    uint8_t devad, uint16_t reg);
static int mtk_mmd_ind_write(struct mtk_eth_priv *priv, uint8_t addr,
			     uint8_t devad, uint16_t reg, uint16_t val);
#elif defined(SWITCH_MT7531)
#define SWITCH_RESET_WAIT_MS	200
#define SWITCH_MT7531_COMMON
#elif defined(SWITCH_MT7988)
#define SWITCH_RESET_WAIT_MS	50
#define SWITCH_MT7531_COMMON
#else
#define PHY_MODE
#error PHY mode is not supported
#endif

#if defined(SWITCH_MT7531_COMMON)
#define mt753x_mii_read		mt7531_mii_ind_read
#define mt753x_mii_write	mt7531_mii_ind_write
static int mt7531_mii_ind_read(struct mtk_eth_priv *priv, uint8_t phy,
			       uint8_t reg);
static int mt7531_mii_ind_write(struct mtk_eth_priv *priv, uint8_t phy,
				uint8_t reg, uint16_t val);
#if defined(SWITCH_MT7531)
#define mt753x_mmd_read		mt7531_mmd_ind_read
#define mt753x_mmd_write	mt7531_mmd_ind_write
static int mt7531_mmd_ind_read(struct mtk_eth_priv *priv, uint8_t addr,
			       uint8_t devad, uint16_t reg);
static int mt7531_mmd_ind_write(struct mtk_eth_priv *priv, uint8_t addr,
				uint8_t devad, uint16_t reg, uint16_t val);
#endif
#endif

static struct mtk_eth_priv _eth_priv;
static bool eth_ready;

static void mtk_pdma_write(struct mtk_eth_priv *priv, uint32_t reg,
			   uint32_t val)
{
	mmio_write_32(FE_BASE + PDMA_BASE + reg, val);
}

static void mtk_pdma_rmw(struct mtk_eth_priv *priv, uint32_t reg, uint32_t clr,
			 uint32_t set)
{
	mmio_clrsetbits_32(FE_BASE + PDMA_BASE + reg, clr, set);
}

static void mtk_gdma_write(struct mtk_eth_priv *priv, int no, uint32_t reg,
			   uint32_t val)
{
	uint32_t gdma_base;

	if (no == 2)
		gdma_base = GDMA3_BASE;
	else if (no == 1)
		gdma_base = GDMA2_BASE;
	else
		gdma_base = GDMA1_BASE;

	mmio_write_32(FE_BASE + gdma_base + reg, val);
}

#if defined(SWITCH_MT7988)
static void mtk_fe_rmw(struct mtk_eth_priv *priv, uint32_t reg, uint32_t clr,
		       uint32_t set)
{
	mmio_clrsetbits_32(FE_BASE + reg, clr, set);
}
#endif

static uint32_t mtk_gmac_read(struct mtk_eth_priv *priv, uint32_t reg)
{
	return mmio_read_32(FE_BASE + GMAC_BASE + reg);
}

static void mtk_gmac_write(struct mtk_eth_priv *priv, uint32_t reg,
			   uint32_t val)
{
	mmio_write_32(FE_BASE + GMAC_BASE + reg, val);
}

static void mtk_gmac_rmw(struct mtk_eth_priv *priv, uint32_t reg, uint32_t clr,
			 uint32_t set)
{
	mmio_clrsetbits_32(FE_BASE + GMAC_BASE + reg, clr, set);
}

static void mtk_ethsys_rmw(struct mtk_eth_priv *priv, uint32_t reg,
			   uint32_t clr, uint32_t set)
{
	mmio_clrsetbits_32(ETHSYS_BASE + reg, clr, set);
}

static void __maybe_unused mtk_infra_rmw(struct mtk_eth_priv *priv,
					 uint32_t reg, uint32_t clr,
					 uint32_t set)
{
	mmio_clrsetbits_32(TOP_MISC_BASE + reg, clr, set);
}

#if defined(SWITCH_MT7988)
static uint32_t mtk_gsw_read(struct mtk_eth_priv *priv, uint32_t reg)
{
	return mmio_read_32(ETHSYS_BASE + GSW_BASE + reg);
}

static void mtk_gsw_write(struct mtk_eth_priv *priv, uint32_t reg, uint32_t val)
{
	mmio_write_32(ETHSYS_BASE + GSW_BASE + reg, val);
}
#endif

/* Direct MDIO clause 22/45 access via SoC */
static int mtk_mii_rw(struct mtk_eth_priv *priv, uint8_t phy, uint8_t reg,
		      uint16_t data, uint32_t cmd, uint32_t st)
{
	int ret;
	uint32_t val;

	val = (st << MDIO_ST_S) |
	      ((cmd << MDIO_CMD_S) & MDIO_CMD_M) |
	      (((uint32_t)phy << MDIO_PHY_ADDR_S) & MDIO_PHY_ADDR_M) |
	      (((uint32_t)reg << MDIO_REG_ADDR_S) & MDIO_REG_ADDR_M);

	if (cmd == MDIO_CMD_WRITE || cmd == MDIO_CMD_ADDR)
		val |= data & MDIO_RW_DATA_M;

	mtk_gmac_write(priv, GMAC_PIAC_REG, val | PHY_ACS_ST);

	ret = wait_for_bit_32(FE_BASE + GMAC_BASE + GMAC_PIAC_REG,
			      PHY_ACS_ST, false, 5000);
	if (ret) {
		WARN("MDIO access timeout\n");
		return ret;
	}

	if (cmd == MDIO_CMD_READ || cmd == MDIO_CMD_READ_C45) {
		val = mtk_gmac_read(priv, GMAC_PIAC_REG);
		return val & MDIO_RW_DATA_M;
	}

	return 0;
}

/* Direct MDIO clause 22 read via SoC */
static int mtk_mii_read(struct mtk_eth_priv *priv, uint8_t phy, uint8_t reg)
{
	return mtk_mii_rw(priv, phy, reg, 0, MDIO_CMD_READ, MDIO_ST_C22);
}

/* Direct MDIO clause 22 write via SoC */
static int mtk_mii_write(struct mtk_eth_priv *priv, uint8_t phy, uint8_t reg,
			 uint16_t data)
{
	return mtk_mii_rw(priv, phy, reg, data, MDIO_CMD_WRITE, MDIO_ST_C22);
}

/* Direct MDIO clause 45 read via SoC */
static int mtk_mmd_read(struct mtk_eth_priv *priv, uint8_t addr, uint8_t devad,
			uint16_t reg)
{
	int ret;

	ret = mtk_mii_rw(priv, addr, devad, reg, MDIO_CMD_ADDR, MDIO_ST_C45);
	if (ret)
		return ret;

	return mtk_mii_rw(priv, addr, devad, 0, MDIO_CMD_READ_C45,
			  MDIO_ST_C45);
}

/* Direct MDIO clause 45 write via SoC */
static int mtk_mmd_write(struct mtk_eth_priv *priv, uint8_t addr, uint8_t devad,
			 uint16_t reg, uint16_t val)
{
	int ret;

	ret = mtk_mii_rw(priv, addr, devad, reg, MDIO_CMD_ADDR, MDIO_ST_C45);
	if (ret)
		return ret;

	return mtk_mii_rw(priv, addr, devad, val, MDIO_CMD_WRITE,
			  MDIO_ST_C45);
}

#if defined(SWITCH_MT7530)
/* Indirect MDIO clause 45 read via MII registers */
static int mtk_mmd_ind_read(struct mtk_eth_priv *priv, uint8_t addr,
			    uint8_t devad, uint16_t reg)
{
	int ret;

	ret = mtk_mii_write(priv, addr, MII_MMD_ACC_CTL_REG,
			    (MMD_ADDR << MMD_CMD_S) |
			    ((devad << MMD_DEVAD_S) & MMD_DEVAD_M));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, addr, MII_MMD_ADDR_DATA_REG, reg);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, addr, MII_MMD_ACC_CTL_REG,
			    (MMD_DATA << MMD_CMD_S) |
			    ((devad << MMD_DEVAD_S) & MMD_DEVAD_M));
	if (ret)
		return ret;

	return mtk_mii_read(priv, addr, MII_MMD_ADDR_DATA_REG);
}

/* Indirect MDIO clause 45 write via MII registers */
static int mtk_mmd_ind_write(struct mtk_eth_priv *priv, uint8_t addr,
			     uint8_t devad, uint16_t reg, uint16_t val)
{
	int ret;

	ret = mtk_mii_write(priv, addr, MII_MMD_ACC_CTL_REG,
			    (MMD_ADDR << MMD_CMD_S) |
			    ((devad << MMD_DEVAD_S) & MMD_DEVAD_M));
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, addr, MII_MMD_ADDR_DATA_REG, reg);
	if (ret)
		return ret;

	ret = mtk_mii_write(priv, addr, MII_MMD_ACC_CTL_REG,
			    (MMD_DATA << MMD_CMD_S) |
			    ((devad << MMD_DEVAD_S) & MMD_DEVAD_M));
	if (ret)
		return ret;

	return mtk_mii_write(priv, addr, MII_MMD_ADDR_DATA_REG, val);
}
#endif /* SWITCH_MT7530 */

#if !defined(PHY_MODE)
/*
 * MT7530 Internal Register Address Bits
 * -------------------------------------------------------------------
 * | 15  14  13  12  11  10   9   8   7   6 | 5   4   3   2 | 1   0  |
 * |----------------------------------------|---------------|--------|
 * |              Page Address              |  Reg Address  | Unused |
 * -------------------------------------------------------------------
 */

static int mt753x_reg_read(struct mtk_eth_priv *priv, uint32_t reg,
			   uint32_t *data)
{
	int ret, low_word, high_word;

#if defined(SWITCH_MT7988)
	*data = mtk_gsw_read(priv, reg);
	return 0;
#endif

	/* Write page address */
	ret = mtk_mii_write(priv, MT753X_DFL_SMI_ADDR, 0x1f, reg >> 6);
	if (ret)
		return ret;

	/* Read low word */
	low_word = mtk_mii_read(priv, MT753X_DFL_SMI_ADDR, (reg >> 2) & 0xf);
	if (low_word < 0)
		return low_word;

	/* Read high word */
	high_word = mtk_mii_read(priv, MT753X_DFL_SMI_ADDR, 0x10);
	if (high_word < 0)
		return high_word;

	if (data)
		*data = ((uint32_t)high_word << 16) | (low_word & 0xffff);

	return 0;
}

static int mt753x_reg_write(struct mtk_eth_priv *priv, uint32_t reg,
			    uint32_t data)
{
	int ret;

#if defined(SWITCH_MT7988)
	mtk_gsw_write(priv, reg, data);
	return 0;
#endif

	/* Write page address */
	ret = mtk_mii_write(priv, MT753X_DFL_SMI_ADDR, 0x1f, reg >> 6);
	if (ret)
		return ret;

	/* Write low word */
	ret = mtk_mii_write(priv, MT753X_DFL_SMI_ADDR, (reg >> 2) & 0xf,
			    data & 0xffff);
	if (ret)
		return ret;

	/* Write high word */
	return mtk_mii_write(priv, MT753X_DFL_SMI_ADDR, 0x10, data >> 16);
}

static void __maybe_unused mt753x_reg_rmw(struct mtk_eth_priv *priv,
					  uint32_t reg, uint32_t clr,
					  uint32_t set)
{
	uint32_t val;

	mt753x_reg_read(priv, reg, &val);
	val &= ~clr;
	val |= set;
	mt753x_reg_write(priv, reg, val);
}
#endif /* !PHY_MODE */

#if defined(SWITCH_MT7531_COMMON)
/* Indirect MDIO clause 22/45 access */
static int mt7531_mii_rw(struct mtk_eth_priv *priv, int phy, int reg,
			 uint16_t data, uint32_t cmd, uint32_t st)
{
	uint64_t timeout;
	uint32_t val, timeout_ms;
	int ret = 0;

	val = (st << MDIO_ST_S) |
	      ((cmd << MDIO_CMD_S) & MDIO_CMD_M) |
	      ((phy << MDIO_PHY_ADDR_S) & MDIO_PHY_ADDR_M) |
	      ((reg << MDIO_REG_ADDR_S) & MDIO_REG_ADDR_M);

	if (cmd == MDIO_CMD_WRITE || cmd == MDIO_CMD_ADDR)
		val |= data & MDIO_RW_DATA_M;

	mt753x_reg_write(priv, MT7531_PHY_IAC, val | PHY_ACS_ST);

	timeout_ms = 100;
	timeout = timeout_init_us(timeout_ms * 1000);
	while (1) {
		mt753x_reg_read(priv, MT7531_PHY_IAC, &val);

		if ((val & PHY_ACS_ST) == 0)
			break;

		if (timeout_elapsed(timeout))
			return -ETIMEDOUT;
	}

	if (cmd == MDIO_CMD_READ || cmd == MDIO_CMD_READ_C45) {
		mt753x_reg_read(priv, MT7531_PHY_IAC, &val);
		ret = val & MDIO_RW_DATA_M;
	}

	return ret;
}

static int mt7531_mii_ind_read(struct mtk_eth_priv *priv, uint8_t phy,
			       uint8_t reg)
{
	uint8_t phy_addr;

	if (phy >= MT753X_NUM_PHYS)
		return -EINVAL;

	phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, phy);

	return mt7531_mii_rw(priv, phy_addr, reg, 0, MDIO_CMD_READ,
			     MDIO_ST_C22);
}

static int mt7531_mii_ind_write(struct mtk_eth_priv *priv, uint8_t phy,
				uint8_t reg, uint16_t val)
{
	uint8_t phy_addr;

	if (phy >= MT753X_NUM_PHYS)
		return -EINVAL;

	phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, phy);

	return mt7531_mii_rw(priv, phy_addr, reg, val, MDIO_CMD_WRITE,
			     MDIO_ST_C22);
}

#if defined(SWITCH_MT7531)
static int mt7531_mmd_ind_read(struct mtk_eth_priv *priv, uint8_t addr,
			       uint8_t devad, uint16_t reg)
{
	uint8_t phy_addr;
	int ret;

	if (addr >= MT753X_NUM_PHYS)
		return -EINVAL;

	phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, addr);

	ret = mt7531_mii_rw(priv, phy_addr, devad, reg, MDIO_CMD_ADDR,
			    MDIO_ST_C45);
	if (ret)
		return ret;

	return mt7531_mii_rw(priv, phy_addr, devad, 0, MDIO_CMD_READ_C45,
			     MDIO_ST_C45);
}

static int mt7531_mmd_ind_write(struct mtk_eth_priv *priv, uint8_t addr,
				uint8_t devad, uint16_t reg, uint16_t val)
{
	uint8_t phy_addr;
	int ret;

	if (addr >= MT753X_NUM_PHYS)
		return 0;

	phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, addr);

	ret = mt7531_mii_rw(priv, phy_addr, devad, reg, MDIO_CMD_ADDR,
			    MDIO_ST_C45);
	if (ret)
		return ret;

	return mt7531_mii_rw(priv, phy_addr, devad, val, MDIO_CMD_WRITE,
			     MDIO_ST_C45);
}
#endif
#endif /* SWITCH_MT7531_COMMON */

#if defined(SWITCH_MT7530) || defined(SWITCH_MT7531)
static int mt753x_core_reg_read(struct mtk_eth_priv *priv, uint32_t reg)
{
	uint8_t phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, 0);

	return mt753x_mmd_read(priv, phy_addr, 0x1f, reg);
}

static void mt753x_core_reg_write(struct mtk_eth_priv *priv, uint32_t reg,
				  uint32_t val)
{
	uint8_t phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, 0);

	mt753x_mmd_write(priv, phy_addr, 0x1f, reg, val);
}
#endif /* SWITCH_MT7530 || SWITCH_MT7531 */

#if defined(SWITCH_MT7530)
static int mt7530_pad_clk_setup(struct mtk_eth_priv *priv)
{
	uint32_t ncpo1, ssc_delta;

#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_RGMII
	ncpo1 = 0x0c80;
	ssc_delta = 0x87;
#else
#error Unsupported interface mode
#endif

	/* Disable MT7530 core clock */
	mt753x_core_reg_write(priv, CORE_TRGMII_GSW_CLK_CG, 0);

	/* Disable MT7530 PLL */
	mt753x_core_reg_write(priv, CORE_GSWPLL_GRP1,
			      (2 << RG_GSWPLL_POSDIV_200M_S) |
			      (32 << RG_GSWPLL_FBKDIV_200M_S));

	/* For MT7530 core clock = 500Mhz */
	mt753x_core_reg_write(priv, CORE_GSWPLL_GRP2,
			      (1 << RG_GSWPLL_POSDIV_500M_S) |
			      (25 << RG_GSWPLL_FBKDIV_500M_S));

	/* Enable MT7530 PLL */
	mt753x_core_reg_write(priv, CORE_GSWPLL_GRP1,
			      (2 << RG_GSWPLL_POSDIV_200M_S) |
			      (32 << RG_GSWPLL_FBKDIV_200M_S) |
			      RG_GSWPLL_EN_PRE);

	udelay(20);

	mt753x_core_reg_write(priv, CORE_TRGMII_GSW_CLK_CG, REG_GSWCK_EN);

	/* Setup the MT7530 TRGMII Tx Clock */
	mt753x_core_reg_write(priv, CORE_PLL_GROUP5, ncpo1);
	mt753x_core_reg_write(priv, CORE_PLL_GROUP6, 0);
	mt753x_core_reg_write(priv, CORE_PLL_GROUP10, ssc_delta);
	mt753x_core_reg_write(priv, CORE_PLL_GROUP11, ssc_delta);
	mt753x_core_reg_write(priv, CORE_PLL_GROUP4, RG_SYSPLL_DDSFBK_EN |
			      RG_SYSPLL_BIAS_EN | RG_SYSPLL_BIAS_LPF_EN);

	mt753x_core_reg_write(priv, CORE_PLL_GROUP2,
			      RG_SYSPLL_EN_NORMAL | RG_SYSPLL_VODEN |
			      (1 << RG_SYSPLL_POSDIV_S));

	mt753x_core_reg_write(priv, CORE_PLL_GROUP7,
			      RG_LCDDS_PCW_NCPO_CHG | (3 << RG_LCCDS_C_S) |
			      RG_LCDDS_PWDB | RG_LCDDS_ISO_EN);

	/* Enable MT7530 core clock */
	mt753x_core_reg_write(priv, CORE_TRGMII_GSW_CLK_CG,
			      REG_GSWCK_EN | REG_TRGMIICK_EN);

	return 0;
}

static void mt753x_mac_control(struct mtk_eth_priv *priv, bool enable)
{
	uint32_t pmcr = FORCE_MODE;

	if (enable)
		pmcr = priv->mt753x_pmcr;

	mt753x_reg_write(priv, PMCR_REG(6), pmcr);
}

static int mt753x_setup(struct mtk_eth_priv *priv)
{
	uint16_t phy_addr, phy_val;
	uint32_t val, txdrv;
	int i;

	if (!MTK_HAS_CAPS(SOC_CAPS, MTK_TRGMII_MT7621_CLK)) {
		/* Select 250MHz clk for RGMII mode */
		mtk_ethsys_rmw(priv, ETHSYS_CLKCFG0_REG,
			       ETHSYS_TRGMII_CLK_SEL362_5, 0);

		txdrv = 8;
	} else {
		txdrv = 4;
	}

	/* Modify HWTRAP first to allow direct access to internal PHYs */
	mt753x_reg_read(priv, HWTRAP_REG, &val);
	val |= CHG_TRAP;
	val &= ~C_MDIO_BPS;
	mt753x_reg_write(priv, MHWTRAP_REG, val);

	/* Calculate the phy base address */
	val = ((val & SMI_ADDR_M) >> SMI_ADDR_S) << 3;
	priv->mt753x_phy_base = (val | 0x7) + 1;

	/* Turn off PHYs */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, i);
		phy_val = mt753x_mii_read(priv, phy_addr, MII_BMCR);
		phy_val |= BMCR_PDOWN;
		mt753x_mii_write(priv, phy_addr, MII_BMCR, phy_val);
	}

	/* Force MAC link down before reset */
	mt753x_reg_write(priv, PMCR_REG(5), FORCE_MODE);
	mt753x_reg_write(priv, PMCR_REG(6), FORCE_MODE);

	/* MT7530 reset */
	mt753x_reg_write(priv, SYS_CTRL_REG, SW_SYS_RST | SW_REG_RST);
	udelay(100);

	val = (IPG_96BIT_WITH_SHORT_IPG << IPG_CFG_S) |
	      MAC_MODE | FORCE_MODE |
	      MAC_TX_EN | MAC_RX_EN |
	      BKOFF_EN | BACKPR_EN |
	      (SPEED_1000M << FORCE_SPD_S) |
	      FORCE_DPX | FORCE_LINK;

	/* MT7530 Port6: Forced 1000M/FD, FC disabled */
	priv->mt753x_pmcr = val;

	/* MT7530 Port5: Forced link down */
	mt753x_reg_write(priv, PMCR_REG(5), FORCE_MODE);

	/* Keep MAC link down before starting eth */
	mt753x_reg_write(priv, PMCR_REG(6), FORCE_MODE);

	/* MT7530 Port6: Set to RGMII */
	mt753x_reg_rmw(priv, MT7530_P6ECR, P6_INTF_MODE_M, P6_INTF_MODE_RGMII);

	/* Hardware Trap: Enable Port6, Disable Port5 */
	mt753x_reg_read(priv, HWTRAP_REG, &val);
	val |= CHG_TRAP | LOOPDET_DIS | P5_INTF_DIS |
	       (P5_INTF_SEL_GMAC5 << P5_INTF_SEL_S) |
	       (P5_INTF_MODE_RGMII << P5_INTF_MODE_S);
	val &= ~(C_MDIO_BPS | P6_INTF_DIS);
	mt753x_reg_write(priv, MHWTRAP_REG, val);

	/* Setup switch core pll */
	mt7530_pad_clk_setup(priv);

	/* Lower Tx Driving for TRGMII path */
	for (i = 0 ; i < NUM_TRGMII_CTRL ; i++)
		mt753x_reg_write(priv, MT7530_TRGMII_TD_ODT(i),
				 (txdrv << TD_DM_DRVP_S) |
				 (txdrv << TD_DM_DRVN_S));

	for (i = 0 ; i < NUM_TRGMII_CTRL; i++)
		mt753x_reg_rmw(priv, MT7530_TRGMII_RD(i), RD_TAP_M, 16);

	/* Turn on PHYs */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, i);
		phy_val = mt753x_mii_read(priv, phy_addr, MII_BMCR);
		phy_val &= ~BMCR_PDOWN;
		mt753x_mii_write(priv, phy_addr, MII_BMCR, phy_val);
	}

	return 0;
}
#elif defined(SWITCH_MT7531)
static void mt7531_core_pll_setup(struct mtk_eth_priv *priv, int mcm)
{
	/* Step 1 : Disable MT7531 COREPLL */
	mt753x_reg_rmw(priv, MT7531_PLLGP_EN, EN_COREPLL, 0);

	/* Step 2: switch to XTAL output */
	mt753x_reg_rmw(priv, MT7531_PLLGP_EN, SW_CLKSW, SW_CLKSW);

	mt753x_reg_rmw(priv, MT7531_PLLGP_CR0, RG_COREPLL_EN, 0);

	/* Step 3: disable PLLGP and enable program PLLGP */
	mt753x_reg_rmw(priv, MT7531_PLLGP_EN, SW_PLLGP, SW_PLLGP);

	/* Step 4: program COREPLL output frequency to 500MHz */
	mt753x_reg_rmw(priv, MT7531_PLLGP_CR0, RG_COREPLL_POSDIV_M,
		       2 << RG_COREPLL_POSDIV_S);
	udelay(25);

	/* Currently, support XTAL 25Mhz only */
	mt753x_reg_rmw(priv, MT7531_PLLGP_CR0, RG_COREPLL_SDM_PCW_M,
		       0x140000 << RG_COREPLL_SDM_PCW_S);

	/* Set feedback divide ratio update signal to high */
	mt753x_reg_rmw(priv, MT7531_PLLGP_CR0, RG_COREPLL_SDM_PCW_CHG,
		       RG_COREPLL_SDM_PCW_CHG);

	/* Wait for at least 16 XTAL clocks */
	udelay(10);

	/* Step 5: set feedback divide ratio update signal to low */
	mt753x_reg_rmw(priv, MT7531_PLLGP_CR0, RG_COREPLL_SDM_PCW_CHG, 0);

	/* add enable 325M clock for SGMII */
	mt753x_reg_write(priv, MT7531_ANA_PLLGP_CR5, 0xad0000);

	/* add enable 250SSC clock for RGMII */
	mt753x_reg_write(priv, MT7531_ANA_PLLGP_CR2, 0x4f40000);

	/*Step 6: Enable MT7531 PLL */
	mt753x_reg_rmw(priv, MT7531_PLLGP_CR0, RG_COREPLL_EN, RG_COREPLL_EN);

	mt753x_reg_rmw(priv, MT7531_PLLGP_EN, EN_COREPLL, EN_COREPLL);

	udelay(25);
}

static int mt7531_port_sgmii_init(struct mtk_eth_priv *priv,
				  uint32_t port)
{
	if (port != 5 && port != 6) {
		ERROR("mt7531: port %d is not a SGMII port\n", port);
		return -EINVAL;
	}

	/* Set SGMII GEN2 speed(2.5G) */
	mt753x_reg_rmw(priv, MT7531_PHYA_CTRL_SIGNAL3(port),
		       SGMSYS_SPEED_2500, SGMSYS_SPEED_2500);

	/* Disable SGMII AN */
	mt753x_reg_rmw(priv, MT7531_PCS_CONTROL_1(port),
		       SGMII_AN_ENABLE, 0);

	/* SGMII force mode setting */
	mt753x_reg_write(priv, MT7531_SGMII_MODE(port), SGMII_FORCE_MODE);

	/* Release PHYA power down state */
	mt753x_reg_rmw(priv, MT7531_QPHY_PWR_STATE_CTRL(port),
		       SGMII_PHYA_PWD, 0);

	return 0;
}

#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_RGMII
static int mt7531_port_rgmii_init(struct mtk_eth_priv *priv, uint32_t port)
{
	uint32_t val;

	if (port != 5) {
		ERROR("error: RGMII mode is not available for port %d\n",
		       port);
		return -EINVAL;
	}

	mt753x_reg_read(priv, MT7531_CLKGEN_CTRL, &val);
	val |= GP_CLK_EN;
	val &= ~GP_MODE_M;
	val |= GP_MODE_RGMII << GP_MODE_S;
	val |= TXCLK_NO_REVERSE;
	val |= RXCLK_NO_DELAY;
	val &= ~CLK_SKEW_IN_M;
	val |= CLK_SKEW_IN_NO_CHANGE << CLK_SKEW_IN_S;
	val &= ~CLK_SKEW_OUT_M;
	val |= CLK_SKEW_OUT_NO_CHANGE << CLK_SKEW_OUT_S;
	mt753x_reg_write(priv, MT7531_CLKGEN_CTRL, val);

	return 0;
}
#endif /* PHY_INTERFACE_MODE_RGMII */

static void mt7531_phy_setting(struct mtk_eth_priv *priv)
{
	int i;
	uint32_t val;

	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		/* Enable HW auto downshift */
		mt753x_mii_write(priv, i, 0x1f, 0x1);
		val = mt753x_mii_read(priv, i, PHY_EXT_REG_14);
		val |= PHY_EN_DOWN_SHFIT;
		mt753x_mii_write(priv, i, PHY_EXT_REG_14, val);

		/* PHY link down power saving enable */
		val = mt753x_mii_read(priv, i, PHY_EXT_REG_17);
		val |= PHY_LINKDOWN_POWER_SAVING_EN;
		mt753x_mii_write(priv, i, PHY_EXT_REG_17, val);

		val = mt753x_mmd_read(priv, i, 0x1e, PHY_DEV1E_REG_0C6);
		val &= ~PHY_POWER_SAVING_M;
		val |= PHY_POWER_SAVING_TX << PHY_POWER_SAVING_S;
		mt753x_mmd_write(priv, i, 0x1e, PHY_DEV1E_REG_0C6, val);
	}
}

static void mt753x_mac_control(struct mtk_eth_priv *priv, bool enable)
{
	uint32_t pmcr = FORCE_MODE_LNK;

	if (enable)
		pmcr = priv->mt753x_pmcr;

	mt753x_reg_write(priv, PMCR_REG(5), pmcr);
	mt753x_reg_write(priv, PMCR_REG(6), pmcr);
}

static int mt753x_setup(struct mtk_eth_priv *priv)
{
	uint16_t phy_addr, phy_val;
	uint32_t val;
	uint32_t pmcr;
	uint32_t port5_sgmii;
	int i;

	priv->mt753x_phy_base = (MT753X_DFL_SMI_ADDR + 1) &
				MT753X_SMI_ADDR_MASK;

	/* Turn off PHYs */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, i);
		phy_val = mt753x_mii_read(priv, phy_addr, MII_BMCR);
		phy_val |= BMCR_PDOWN;
		mt753x_mii_write(priv, phy_addr, MII_BMCR, phy_val);
	}

	/* Force MAC link down before reset */
	mt753x_reg_write(priv, PMCR_REG(5), FORCE_MODE_LNK);
	mt753x_reg_write(priv, PMCR_REG(6), FORCE_MODE_LNK);

	/* Switch soft reset */
	mt753x_reg_write(priv, SYS_CTRL_REG, SW_SYS_RST | SW_REG_RST);
	udelay(100);

	/* Enable MDC input Schmitt Trigger */
	mt753x_reg_rmw(priv, MT7531_SMT0_IOLB, SMT_IOLB_5_SMI_MDC_EN,
		       SMT_IOLB_5_SMI_MDC_EN);

	mt7531_core_pll_setup(priv, 0);

	mt753x_reg_read(priv, MT7531_TOP_SIG_SR, &val);
	port5_sgmii = !!(val & PAD_DUAL_SGMII_EN);

	/* port5 support either RGMII or SGMII, port6 only support SGMII. */
#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_RGMII
	if (!port5_sgmii)
		mt7531_port_rgmii_init(priv, 5);
#elif GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_2500BASEX
	mt7531_port_sgmii_init(priv, 6);
	if (port5_sgmii)
		mt7531_port_sgmii_init(priv, 5);
#endif

	pmcr = MT7531_FORCE_MODE |
	       (IPG_96BIT_WITH_SHORT_IPG << IPG_CFG_S) |
	       MAC_MODE | MAC_TX_EN | MAC_RX_EN |
	       BKOFF_EN | BACKPR_EN |
	       FORCE_RX_FC | FORCE_TX_FC |
	       (SPEED_1000M << FORCE_SPD_S) | FORCE_DPX |
	       FORCE_LINK;

	priv->mt753x_pmcr = pmcr;

	/* Keep MAC link down before starting eth */
	mt753x_reg_write(priv, PMCR_REG(5), FORCE_MODE_LNK);
	mt753x_reg_write(priv, PMCR_REG(6), FORCE_MODE_LNK);

	/* Turn on PHYs */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, i);
		phy_val = mt753x_mii_read(priv, phy_addr, MII_BMCR);
		phy_val &= ~BMCR_PDOWN;
		mt753x_mii_write(priv, phy_addr, MII_BMCR, phy_val);
	}

	mt7531_phy_setting(priv);

	/* Enable Internal PHYs */
	val = mt753x_core_reg_read(priv, CORE_PLL_GROUP4);
	val |= MT7531_BYPASS_MODE;
	val &= ~MT7531_POWER_ON_OFF;
	mt753x_core_reg_write(priv, CORE_PLL_GROUP4, val);

	return 0;
}
#elif defined(SWITCH_MT7988)
static void mt7988_phy_setting(struct mtk_eth_priv *priv)
{
	uint16_t val;
	uint32_t i;

	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		/* Enable HW auto downshift */
		mt753x_mii_write(priv, i, 0x1f, 0x1);
		val = mt753x_mii_read(priv, i, PHY_EXT_REG_14);
		val |= PHY_EN_DOWN_SHFIT;
		mt753x_mii_write(priv, i, PHY_EXT_REG_14, val);

		/* PHY link down power saving enable */
		val = mt753x_mii_read(priv, i, PHY_EXT_REG_17);
		val |= PHY_LINKDOWN_POWER_SAVING_EN;
		mt753x_mii_write(priv, i, PHY_EXT_REG_17, val);
	}
}

static void mt753x_mac_control(struct mtk_eth_priv *priv, bool enable)
{
	uint32_t pmcr = FORCE_MODE_LNK;

	if (enable)
		pmcr = priv->mt753x_pmcr;

	mt753x_reg_write(priv, PMCR_REG(6), pmcr);
}

static int mt753x_setup(struct mtk_eth_priv *priv)
{
	uint16_t phy_addr, phy_val;
	uint32_t pmcr;
	int i;

	priv->mt753x_phy_base = (MT753X_DFL_SMI_ADDR + 1) &
				MT753X_SMI_ADDR_MASK;

	/* Turn off PHYs */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, i);
		phy_val = mt753x_mii_read(priv, phy_addr, MII_BMCR);
		phy_val |= BMCR_PDOWN;
		mt753x_mii_write(priv, phy_addr, MII_BMCR, phy_val);
	}

#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII
	/* Use CPU bridge instead of actual USXGMII path */

	/* Disable GDM1 RX CRC stripping */
	/* mtk_fe_rmw(priv, 0x500, BIT(16), 0); */

	/* Set GDM1 no drop */
	mtk_fe_rmw(priv, PSE_NO_DROP_CFG_REG, 0, PSE_NO_DROP_GDM1);

	/* Enable GSW CPU bridge as USXGMII */
	/* mtk_fe_rmw(priv, 0x504, BIT(31), BIT(31)); */

	/* Enable GDM1 to GSW CPU bridge */
	mtk_gmac_rmw(priv, GMAC_MAC_MISC_REG, 0, BIT(0));

	/* XGMAC force link up */
	mtk_gmac_rmw(priv, GMAC_XGMAC_STS_REG, 0, P1_XGMAC_FORCE_LINK);

	/* Setup GSW CPU bridge IPG */
	mtk_gmac_rmw(priv, GMAC_GSW_CFG_REG, GSWTX_IPG_M | GSWRX_IPG_M,
		     (0xB << GSWTX_IPG_S) | (0xB << GSWRX_IPG_S));
#else
#error Unsupported interface mode
#endif

	pmcr = MT7988_FORCE_MODE |
	       (IPG_96BIT_WITH_SHORT_IPG << IPG_CFG_S) |
	       MAC_MODE | MAC_TX_EN | MAC_RX_EN |
	       BKOFF_EN | BACKPR_EN |
	       FORCE_RX_FC | FORCE_TX_FC |
	       (SPEED_1000M << FORCE_SPD_S) | FORCE_DPX |
	       FORCE_LINK;

	priv->mt753x_pmcr = pmcr;

	/* Keep MAC link down before starting eth */
	mt753x_reg_write(priv, PMCR_REG(6), FORCE_MODE_LNK);

	/* Turn on PHYs */
	for (i = 0; i < MT753X_NUM_PHYS; i++) {
		phy_addr = MT753X_PHY_ADDR(priv->mt753x_phy_base, i);
		phy_val = mt753x_mii_read(priv, phy_addr, MII_BMCR);
		phy_val &= ~BMCR_PDOWN;
		mt753x_mii_write(priv, phy_addr, MII_BMCR, phy_val);
	}

	mt7988_phy_setting(priv);

	return 0;
}
#endif /* SWITCH_MT7xxx */

#if !defined(PHY_MODE)
static int mt753x_switch_init(struct mtk_eth_priv *priv)
{
	uint32_t i;
	int ret;

	/* Global reset switch */
#ifdef SWITCH_RESET_MCM
	mmio_setbits_32(SWITCH_RESET_MCM_REG, BIT(SWITCH_RESET_MCM_BIT));
	mdelay(10);
	mmio_clrbits_32(SWITCH_RESET_MCM_REG, BIT(SWITCH_RESET_MCM_BIT));
	mdelay(SWITCH_RESET_WAIT_MS);
#elif defined(SWITCH_RESET_GPIO)
	mtk_plat_switch_reset(true);	/* Provided in plat_eth_def.h */
	mdelay(10);
	mtk_plat_switch_reset(false);
	mdelay(SWITCH_RESET_WAIT_MS);
#endif

	ret = mt753x_setup(priv);
	if (ret)
		return ret;

	/* Set port isolation */
	for (i = 0; i < MT753X_NUM_PORTS; i++) {
		/* Set port matrix mode */
		if (i != 6)
			mt753x_reg_write(priv, PCR_REG(i),
					 (0x40 << PORT_MATRIX_S));
		else
			mt753x_reg_write(priv, PCR_REG(i),
					 (0x3f << PORT_MATRIX_S));

		/* Set port mode to user port */
		mt753x_reg_write(priv, PVC_REG(i),
				 (0x8100U << STAG_VPID_S) |
				 (VLAN_ATTR_USER << VLAN_ATTR_S));
	}

	return 0;
}

static int mt753x_switch_wait_port_ready(struct mtk_eth_priv *priv,
					 uint32_t timeout_ms)
{
	uint64_t tmo = timeout_init_us(timeout_ms * 1000);
	uint16_t sts;
	uint32_t i;

	while (!timeout_elapsed(tmo)) {
		for (i = 0; i < MT753X_NUM_PHYS; i++) {
			sts = mt753x_mii_read(priv, i, MII_BMSR);
			if (sts & BMSR_ANEGCOMPLETE) {
				if (sts & BMSR_LSTATUS)
					return 0;
			}
		}

		mdelay(100);
	}

	return -ETIMEDOUT;
}
#endif /* !PHY_MODE */

#if defined(PHY_MODE)
static void mtk_xphy_link_adjust(struct mtk_eth_priv *priv)
{
	uint16_t lcl_adv = 0, rmt_adv = 0;
	uint8_t flowctrl;
	uint32_t mcr;

	mcr = mtk_gmac_read(priv, XGMAC_PORT_MCR(GMAC_ID));
	mcr &= ~(XGMAC_FORCE_TX_FC | XGMAC_FORCE_RX_FC);

	if (priv->phydev->duplex) {
		if (priv->phydev->pause)
			rmt_adv = LPA_PAUSE_CAP;
		if (priv->phydev->asym_pause)
			rmt_adv |= LPA_PAUSE_ASYM;

		if (priv->phydev->advertising & ADVERTISED_Pause)
			lcl_adv |= ADVERTISE_PAUSE_CAP;
		if (priv->phydev->advertising & ADVERTISED_Asym_Pause)
			lcl_adv |= ADVERTISE_PAUSE_ASYM;

		flowctrl = mii_resolve_flowctrl_fdx(lcl_adv, rmt_adv);

		if (flowctrl & FLOW_CTRL_TX)
			mcr |= XGMAC_FORCE_TX_FC;
		if (flowctrl & FLOW_CTRL_RX)
			mcr |= XGMAC_FORCE_RX_FC;

		debug("rx pause %s, tx pause %s\n",
		      flowctrl & FLOW_CTRL_RX ? "enabled" : "disabled",
		      flowctrl & FLOW_CTRL_TX ? "enabled" : "disabled");
	}

	mcr &= ~(XGMAC_TRX_DISABLE);
	mtk_gmac_write(priv, XGMAC_PORT_MCR(GMAC_ID), mcr);
}

static void mtk_phy_link_adjust(struct mtk_eth_priv *priv)
{
	uint16_t lcl_adv = 0, rmt_adv = 0;
	uint8_t flowctrl;
	uint32_t mcr;

	mcr = (IPG_96BIT_WITH_SHORT_IPG << IPG_CFG_S) |
	      (MAC_RX_PKT_LEN_1536 << MAC_RX_PKT_LEN_S) |
	      MAC_MODE | FORCE_MODE |
	      MAC_TX_EN | MAC_RX_EN |
	      DEL_RXFIFO_CLR |
	      BKOFF_EN | BACKPR_EN;

	switch (priv->phydev->speed) {
	case SPEED_10:
		mcr |= (SPEED_10M << FORCE_SPD_S);
		break;
	case SPEED_100:
		mcr |= (SPEED_100M << FORCE_SPD_S);
		break;
	case SPEED_1000:
	case SPEED_2500:
		mcr |= (SPEED_1000M << FORCE_SPD_S);
		break;
	};

	if (priv->phydev->link)
		mcr |= FORCE_LINK;

	if (priv->phydev->duplex) {
		mcr |= FORCE_DPX;

		if (priv->phydev->pause)
			rmt_adv = LPA_PAUSE_CAP;
		if (priv->phydev->asym_pause)
			rmt_adv |= LPA_PAUSE_ASYM;

		if (priv->phydev->advertising & ADVERTISED_Pause)
			lcl_adv |= ADVERTISE_PAUSE_CAP;
		if (priv->phydev->advertising & ADVERTISED_Asym_Pause)
			lcl_adv |= ADVERTISE_PAUSE_ASYM;

		flowctrl = mii_resolve_flowctrl_fdx(lcl_adv, rmt_adv);

		if (flowctrl & FLOW_CTRL_TX)
			mcr |= FORCE_TX_FC;
		if (flowctrl & FLOW_CTRL_RX)
			mcr |= FORCE_RX_FC;

		debug("rx pause %s, tx pause %s\n",
		      flowctrl & FLOW_CTRL_RX ? "enabled" : "disabled",
		      flowctrl & FLOW_CTRL_TX ? "enabled" : "disabled");
	}

	mtk_gmac_write(priv, GMAC_PORT_MCR(GMAC_ID), mcr);
}

static int mtk_phy_start(struct mtk_eth_priv *priv)
{
	struct phy_device *phydev = priv->phydev;
	int ret;

	ret = phy_startup(phydev);

	if (ret) {
		debug("Could not initialize PHY %s\n", phydev->dev->name);
		return ret;
	}

	if (!phydev->link) {
		debug("%s: link down.\n", phydev->dev->name);
		return 0;
	}

	if (!GMAC_FORCE_MODE) {
#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII
		mtk_xphy_link_adjust(priv);
#else
		mtk_phy_link_adjust(priv);
#endif
	}

	debug("Speed: %d, %s duplex%s\n", phydev->speed,
	      (phydev->duplex) ? "full" : "half",
	      (phydev->port == PORT_FIBRE) ? ", fiber mode" : "");

	return 0;
}

static int mtk_phy_probe(struct mtk_eth_priv *priv)
{
	struct phy_device *phydev;

	phydev = phy_connect(priv->mdio_bus, PHY_ADDR, dev,
			     GMAC_INTERFACE_MODE);
	if (!phydev)
		return -ENODEV;

	phydev->supported &= PHY_GBIT_FEATURES;
	phydev->advertising = phydev->supported;

	priv->phydev = phydev;
	phy_config(phydev);

	return 0;
}
#endif

#if (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_SGMII) || \
    (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_2500BASEX)
static void mtk_sgmii_an_init(struct mtk_eth_priv *priv)
{
	/* Set SGMII GEN1 speed(1G) */
	mmio_clrbits_32(SGMII_BASE + ANA_RG3_OFFS, SGMSYS_SPEED_2500);

	/* Enable SGMII AN */
	mmio_setbits_32(SGMII_BASE + SGMSYS_PCS_CONTROL_1, SGMII_AN_ENABLE);

	/* SGMII AN mode setting */
	mmio_write_32(SGMII_BASE + SGMSYS_SGMII_MODE, SGMII_AN_MODE);

#if defined(SGMII_PN_SWAP)
	/* SGMII PN SWAP setting */
	mmio_setbits_32(SGMII_BASE + SGMSYS_QPHY_WRAP_CTRL,
			SGMII_PN_SWAP_TX_RX);
#endif

	/* Release PHYA power down state */
	mmio_clrsetbits_32(SGMII_BASE + SGMSYS_QPHY_PWR_STATE_CTRL,
			SGMII_PHYA_PWD, 0);
}

static void mtk_sgmii_force_init(struct mtk_eth_priv *priv)
{
	/* Set SGMII GEN2 speed(2.5G) */
	mmio_setbits_32(SGMII_BASE + ANA_RG3_OFFS, SGMSYS_SPEED_2500);

	/* Disable SGMII AN */
	mmio_clrbits_32(SGMII_BASE + SGMSYS_PCS_CONTROL_1, SGMII_AN_ENABLE);

	/* SGMII force mode setting */
	mmio_write_32(SGMII_BASE + SGMSYS_SGMII_MODE, SGMII_FORCE_MODE);

#if defined(SGMII_PN_SWAP)
	/* SGMII PN SWAP setting */
	mmio_setbits_32(SGMII_BASE + SGMSYS_QPHY_WRAP_CTRL,
			SGMII_PN_SWAP_TX_RX);
#endif

	/* Release PHYA power down state */
	mmio_clrsetbits_32(SGMII_BASE + SGMSYS_QPHY_PWR_STATE_CTRL,
			   SGMII_PHYA_PWD, 0);
}
#endif /* PHY_INTERFACE_MODE_SGMII || PHY_INTERFACE_MODE_2500BASEX */

#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII
static void mtk_xfi_pll_enable(struct mtk_eth_priv *priv)
{
	/* Add software workaround for USXGMII PLL TCL issue */
	mmio_write_32(XFI_PLL_BASE + XFI_PLL_ANA_GLB8, RG_XFI_PLL_ANA_SWWA);
	mmio_setbits_32(XFI_PLL_BASE + XFI_PLL_DIG_GLB8, RG_XFI_PLL_EN);
}

static void mtk_usxgmii_reset(struct mtk_eth_priv *priv)
{
#if GMAC_ID == 1
	mmio_write_32(TOPRGU_BASE + 0xFC, 0x0000A004);
	mmio_write_32(TOPRGU_BASE + 0x18, 0x88F0A004);
	mmio_write_32(TOPRGU_BASE + 0xFC, 0x00000000);
	mmio_write_32(TOPRGU_BASE + 0x18, 0x88F00000);
	mmio_write_32(TOPRGU_BASE + 0x18, 0x00F00000);
#elif GMAC_ID == 2
	mmio_write_32(TOPRGU_BASE + 0xFC, 0x00005002);
	mmio_write_32(TOPRGU_BASE + 0x18, 0x88F05002);
	mmio_write_32(TOPRGU_BASE + 0xFC, 0x00000000);
	mmio_write_32(TOPRGU_BASE + 0x18, 0x88F00000);
	mmio_write_32(TOPRGU_BASE + 0x18, 0x00F00000);
#endif

	mdelay(10);
}

static void mtk_usxgmii_setup_phya_an_10000(struct mtk_eth_priv *priv)
{
	mmio_write_32(USXGMII_BASE + 0x810, 0x000FFE6D);
	mmio_write_32(USXGMII_BASE + 0x818, 0x07B1EC7B);
	mmio_write_32(USXGMII_BASE + 0x80C, 0x30000000);
	udelay(2);
	mmio_write_32(USXGMII_BASE + 0x80C, 0x10000000);
	udelay(2);
	mmio_write_32(USXGMII_BASE + 0x80C, 0x00000000);

	mmio_write_32(XFI_PEXTP_BASE + 0x9024, 0x00C9071C);
	mmio_write_32(XFI_PEXTP_BASE + 0x2020, 0xAA8585AA);
	mmio_write_32(XFI_PEXTP_BASE + 0x2030, 0x0C020707);
	mmio_write_32(XFI_PEXTP_BASE + 0x2034, 0x0E050F0F);
	mmio_write_32(XFI_PEXTP_BASE + 0x2040, 0x00140032);
	mmio_write_32(XFI_PEXTP_BASE + 0x50F0, 0x00C014AA);
	mmio_write_32(XFI_PEXTP_BASE + 0x50E0, 0x3777C12B);
	mmio_write_32(XFI_PEXTP_BASE + 0x506C, 0x005F9CFF);
	mmio_write_32(XFI_PEXTP_BASE + 0x5070, 0x9D9DFAFA);
	mmio_write_32(XFI_PEXTP_BASE + 0x5074, 0x27273F3F);
	mmio_write_32(XFI_PEXTP_BASE + 0x5078, 0xA7883C68);
	mmio_write_32(XFI_PEXTP_BASE + 0x507C, 0x11661166);
	mmio_write_32(XFI_PEXTP_BASE + 0x5080, 0x0E000AAF);
	mmio_write_32(XFI_PEXTP_BASE + 0x5084, 0x08080D0D);
	mmio_write_32(XFI_PEXTP_BASE + 0x5088, 0x02030909);
	mmio_write_32(XFI_PEXTP_BASE + 0x50E4, 0x0C0C0000);
	mmio_write_32(XFI_PEXTP_BASE + 0x50E8, 0x04040000);
	mmio_write_32(XFI_PEXTP_BASE + 0x50EC, 0x0F0F0C06);
	mmio_write_32(XFI_PEXTP_BASE + 0x50A8, 0x506E8C8C);
	mmio_write_32(XFI_PEXTP_BASE + 0x6004, 0x18190000);
	mmio_write_32(XFI_PEXTP_BASE + 0x00F8, 0x01423342);
	mmio_write_32(XFI_PEXTP_BASE + 0x00F4, 0x80201F20);
	mmio_write_32(XFI_PEXTP_BASE + 0x0030, 0x00050C00);
	mmio_write_32(XFI_PEXTP_BASE + 0x0070, 0x02002800);
	udelay(2);
	mmio_write_32(XFI_PEXTP_BASE + 0x30B0, 0x00000020);
	mmio_write_32(XFI_PEXTP_BASE + 0x3028, 0x00008A01);
	mmio_write_32(XFI_PEXTP_BASE + 0x302C, 0x0000A884);
	mmio_write_32(XFI_PEXTP_BASE + 0x3024, 0x00083002);
	mmio_write_32(XFI_PEXTP_BASE + 0x3010, 0x00022220);
	mmio_write_32(XFI_PEXTP_BASE + 0x5064, 0x0F020A01);
	mmio_write_32(XFI_PEXTP_BASE + 0x50B4, 0x06100600);
	mmio_write_32(XFI_PEXTP_BASE + 0x3048, 0x40704000);
	mmio_write_32(XFI_PEXTP_BASE + 0x3050, 0xA8000000);
	mmio_write_32(XFI_PEXTP_BASE + 0x3054, 0x000000AA);
	mmio_write_32(XFI_PEXTP_BASE + 0x306C, 0x00000F00);
	mmio_write_32(XFI_PEXTP_BASE + 0xA060, 0x00040000);
	mmio_write_32(XFI_PEXTP_BASE + 0x90D0, 0x00000001);
	mmio_write_32(XFI_PEXTP_BASE + 0x0070, 0x0200E800);
	udelay(150);
	mmio_write_32(XFI_PEXTP_BASE + 0x0070, 0x0200C111);
	udelay(2);
	mmio_write_32(XFI_PEXTP_BASE + 0x0070, 0x0200C101);
	udelay(15);
	mmio_write_32(XFI_PEXTP_BASE + 0x0070, 0x0202C111);
	udelay(2);
	mmio_write_32(XFI_PEXTP_BASE + 0x0070, 0x0202C101);
	udelay(100);
	mmio_write_32(XFI_PEXTP_BASE + 0x30B0, 0x00000030);
	mmio_write_32(XFI_PEXTP_BASE + 0x00F4, 0x80201F00);
	mmio_write_32(XFI_PEXTP_BASE + 0x3040, 0x30000000);
	udelay(400);
}

static void mtk_usxgmii_an_init(struct mtk_eth_priv *priv)
{
#if defined(SWITCH_MT7988) && GMAC_ID == 0
	return;
#endif

	mtk_xfi_pll_enable(priv);
	mtk_usxgmii_reset(priv);
	mtk_usxgmii_setup_phya_an_10000(priv);
}

static void mtk_xmac_init(struct mtk_eth_priv *priv)
{
	mtk_usxgmii_an_init(priv);

	/* Set GMAC to the correct mode */
	mtk_ethsys_rmw(priv, ETHSYS_SYSCFG0_REG,
		       SYSCFG0_GE_MODE_M << SYSCFG0_GE_MODE_S(GMAC_ID), 0);

#if GMAC_ID == 1
	mtk_infra_rmw(priv, TOPMISC_NETSYS_PCS_MUX,
		      NETSYS_PCS_MUX_MASK, MUX_G2_USXGMII_SEL);
#elif GMAC_ID == 2
	mtk_gmac_rmw(priv, XGMAC_STS(GMAC_ID), 0, XGMAC_FORCE_LINK);
#endif

	/* Force GMAC link down */
	mtk_gmac_write(priv, GMAC_PORT_MCR(GMAC_ID), FORCE_MODE);
}
#else
static void mtk_mac_init(struct mtk_eth_priv *priv)
{
	int i, ge_mode = 0;
	uint32_t mcr;

#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_RGMII
	ge_mode = GE_MODE_RGMII;
#elif (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_SGMII) || \
      (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_2500BASEX)
	if (MTK_HAS_CAPS(SOC_CAPS, MTK_GMAC2_U3_QPHY)) {
		mtk_infra_rmw(priv, USB_PHY_SWITCH_REG, QPHY_SEL_MASK,
			      SGMII_QPHY_SEL);
	}

	ge_mode = GE_MODE_RGMII;
	mtk_ethsys_rmw(priv, ETHSYS_SYSCFG0_REG, SYSCFG0_SGMII_SEL_M,
		       SYSCFG0_SGMII_SEL(GMAC_ID));
	if (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_SGMII)
		mtk_sgmii_an_init(priv);
	else
		mtk_sgmii_force_init(priv);
#endif

	/* set the gmac to the right mode */
	mtk_ethsys_rmw(priv, ETHSYS_SYSCFG0_REG,
		       SYSCFG0_GE_MODE_M << SYSCFG0_GE_MODE_S(GMAC_ID),
		       ge_mode << SYSCFG0_GE_MODE_S(GMAC_ID));

	if (GMAC_FORCE_MODE) {
		mcr = (IPG_96BIT_WITH_SHORT_IPG << IPG_CFG_S) |
		      (MAC_RX_PKT_LEN_1536 << MAC_RX_PKT_LEN_S) |
		      MAC_MODE | FORCE_MODE |
		      MAC_TX_EN | MAC_RX_EN |
		      BKOFF_EN | BACKPR_EN |
		      FORCE_LINK;

		switch (GMAC_FORCE_SPEED) {
		case SPEED_10:
			mcr |= SPEED_10M << FORCE_SPD_S;
			break;
		case SPEED_100:
			mcr |= SPEED_100M << FORCE_SPD_S;
			break;
		case SPEED_1000:
		case SPEED_2500:
			mcr |= SPEED_1000M << FORCE_SPD_S;
			break;
		}

		if (GMAC_FORCE_FULL_DUPLEX)
			mcr |= FORCE_DPX;

		mtk_gmac_write(priv, GMAC_PORT_MCR(GMAC_ID), mcr);
	}

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_GMAC1_TRGMII) &&
	    !MTK_HAS_CAPS(SOC_CAPS, MTK_TRGMII_MT7621_CLK)) {
		/* Lower Tx Driving for TRGMII path */
		for (i = 0 ; i < NUM_TRGMII_CTRL; i++)
			mtk_gmac_write(priv, GMAC_TRGMII_TD_ODT(i),
				       (8 << TD_DM_DRVP_S) |
				       (8 << TD_DM_DRVN_S));

		mtk_gmac_rmw(priv, GMAC_TRGMII_RCK_CTRL, 0,
			     RX_RST | RXC_DQSISEL);
		mtk_gmac_rmw(priv, GMAC_TRGMII_RCK_CTRL, RX_RST, 0);
	}
}
#endif /* PHY_INTERFACE_MODE_USXGMII */

static void mtk_eth_fifo_init(struct mtk_eth_priv *priv)
{
	uintptr_t pkt_base = priv->pkt_pool_pa;
	struct mtk_tx_dma_v2 *txd;
	struct mtk_rx_dma_v2 *rxd;
	int i;

	mtk_pdma_rmw(priv, PDMA_GLO_CFG_REG, 0xffff0000, 0);
	udelay(500);

	memset(priv->tx_ring_noc, 0, NUM_TX_DESC * TXD_SIZE);
	memset(priv->rx_ring_noc, 0, NUM_RX_DESC * RXD_SIZE);
	memset((void *)priv->pkt_pool_va, 0xff, PKT_TOTAL_BUF_SIZE);

	flush_dcache_range(priv->pkt_pool_va, PKT_TOTAL_BUF_SIZE);

	priv->rx_dma_owner_idx0 = 0;
	priv->tx_cpu_owner_idx0 = 0;

	for (i = 0; i < NUM_TX_DESC; i++) {
		txd = priv->tx_ring_noc + i * TXD_SIZE;

		txd->txd1 = (uint32_t)pkt_base;
		txd->txd2 = PDMA_TXD2_DDONE | PDMA_TXD2_LS0;

		if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
			txd->txd5 = PDMA_V2_TXD5_FPORT_SET(GMAC_ID == 2 ?
							   15 : GMAC_ID + 1);
		else if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2))
			txd->txd5 = PDMA_V2_TXD5_FPORT_SET(GMAC_ID + 1);
		else
			txd->txd4 = PDMA_V1_TXD4_FPORT_SET(GMAC_ID + 1);

		pkt_base += PKTSIZE_ALIGN;
	}

	for (i = 0; i < NUM_RX_DESC; i++) {
		rxd = priv->rx_ring_noc + i * RXD_SIZE;

		rxd->rxd1 = (uint32_t)pkt_base;

		if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2) ||
		    MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
			rxd->rxd2 = PDMA_V2_RXD2_PLEN0_SET(PKTSIZE_ALIGN);
		else
			rxd->rxd2 = PDMA_V1_RXD2_PLEN0_SET(PKTSIZE_ALIGN);

		pkt_base += PKTSIZE_ALIGN;
	}

	mtk_pdma_write(priv, TX_BASE_PTR_REG(0), (uint32_t)priv->tx_ring_pa);
	mtk_pdma_write(priv, TX_MAX_CNT_REG(0), NUM_TX_DESC);
	mtk_pdma_write(priv, TX_CTX_IDX_REG(0), priv->tx_cpu_owner_idx0);

	mtk_pdma_write(priv, RX_BASE_PTR_REG(0), (uint32_t)priv->rx_ring_pa);
	mtk_pdma_write(priv, RX_MAX_CNT_REG(0), NUM_RX_DESC);
	mtk_pdma_write(priv, RX_CRX_IDX_REG(0), NUM_RX_DESC - 1);

	mtk_pdma_write(priv, PDMA_RST_IDX_REG, RST_DTX_IDX0 | RST_DRX_IDX0);
}

int mtk_eth_start(void)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	uint32_t i;

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		mmio_setbits_32(FE_BASE + FE_GLO_MISC_REG, PDMA_VER_V2);

	/* Packets forward to PDMA */
	mtk_gdma_write(priv, GMAC_ID, GDMA_IG_CTRL_REG, GDMA_FWD_TO_CPU);

	for (i = 0; i < GDMA_COUNT; i++) {
		if (i == GMAC_ID)
			continue;

		mtk_gdma_write(priv, i, GDMA_IG_CTRL_REG, GDMA_FWD_DISCARD);
	}

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3)) {
#if defined(SWITCH_MT7988) && GMAC_ID == 0
		mtk_gdma_write(priv, GMAC_ID, GDMA_IG_CTRL_REG,
			       GDMA_BRIDGE_TO_CPU);
#endif

		mtk_gdma_write(priv, GMAC_ID, GDMA_EG_CTRL_REG,
			       GDMA_CPU_BRIDGE_EN);
	}

	udelay(500);

	mtk_eth_fifo_init(priv);

#if !defined(PHY_MODE)
	mt753x_mac_control(priv, true);
#else
	/* Start PHY */
	ret = mtk_phy_start(priv);
	if (ret)
		return ret;
#endif

	mtk_pdma_rmw(priv, PDMA_GLO_CFG_REG, 0,
		     TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN);
	udelay(500);

	return 0;
}

void mtk_eth_stop(void)
{
	struct mtk_eth_priv *priv = &_eth_priv;

#if !defined(PHY_MODE)
	mt753x_mac_control(priv, false);
#endif

	mtk_pdma_rmw(priv, PDMA_GLO_CFG_REG,
		     TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN, 0);
	udelay(500);

	wait_for_bit_32(FE_BASE + PDMA_BASE + PDMA_GLO_CFG_REG,
			RX_DMA_BUSY | TX_DMA_BUSY, false, 5000);
}

void mtk_eth_write_hwaddr(const uint8_t *addr)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	uint32_t macaddr_lsb, macaddr_msb;
	const uint8_t *mac = addr;

	if (!mac)
		return;

	macaddr_msb = ((uint32_t)mac[0] << 8) | (uint32_t)mac[1];
	macaddr_lsb = ((uint32_t)mac[2] << 24) | ((uint32_t)mac[3] << 16) |
		      ((uint32_t)mac[4] << 8) | (uint32_t)mac[5];

	mtk_gdma_write(priv, GMAC_ID, GDMA_MAC_MSB_REG, macaddr_msb);
	mtk_gdma_write(priv, GMAC_ID, GDMA_MAC_LSB_REG, macaddr_lsb);
}

int mtk_eth_send(const void *packet, uint32_t length)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	uint32_t idx = priv->tx_cpu_owner_idx0;
	struct mtk_tx_dma_v2 *txd;
	void *pkt_base;

	txd = priv->tx_ring_noc + idx * TXD_SIZE;

	if (!(txd->txd2 & PDMA_TXD2_DDONE)) {
		VERBOSE("mtk-eth: TX DMA descriptor ring is full\n");
		return -EPERM;
	}

	pkt_base = (void *)((uintptr_t)txd->txd1 - priv->pkt_pool_pa + priv->pkt_pool_va);
	memcpy(pkt_base, packet, length);
	clean_dcache_range((uintptr_t)pkt_base,
			   roundup(length, ARCH_DMA_MINALIGN));

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		txd->txd2 = PDMA_TXD2_LS0 | PDMA_V2_TXD2_SDL0_SET(length);
	else
		txd->txd2 = PDMA_TXD2_LS0 | PDMA_V1_TXD2_SDL0_SET(length);

	priv->tx_cpu_owner_idx0 = (priv->tx_cpu_owner_idx0 + 1) % NUM_TX_DESC;
	mtk_pdma_write(priv, TX_CTX_IDX_REG(0), priv->tx_cpu_owner_idx0);

	return 0;
}

int mtk_eth_recv(void **packetp)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	uint32_t idx = priv->rx_dma_owner_idx0;
	struct mtk_rx_dma_v2 *rxd;
	void *pkt_base;
	uint32_t length;

	rxd = priv->rx_ring_noc + idx * RXD_SIZE;

	if (!(rxd->rxd2 & PDMA_RXD2_DDONE)) {
		VERBOSE("mtk-eth: RX DMA descriptor ring is empty\n");
		return -EAGAIN;
	}

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		length = PDMA_V2_RXD2_PLEN0_GET(rxd->rxd2);
	else
		length = PDMA_V1_RXD2_PLEN0_GET(rxd->rxd2);

	pkt_base = (void *)((uintptr_t)rxd->rxd1 - priv->pkt_pool_pa + priv->pkt_pool_va);
	inv_dcache_range((uintptr_t)pkt_base,
			 roundup(length, ARCH_DMA_MINALIGN));

	if (packetp)
		*packetp = pkt_base;

	return length;
}

int mtk_eth_free_pkt(void *packet)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	uint32_t idx = priv->rx_dma_owner_idx0;
	struct mtk_rx_dma_v2 *rxd;

	rxd = priv->rx_ring_noc + idx * RXD_SIZE;

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		rxd->rxd2 = PDMA_V2_RXD2_PLEN0_SET(PKTSIZE_ALIGN);
	else
		rxd->rxd2 = PDMA_V1_RXD2_PLEN0_SET(PKTSIZE_ALIGN);

	mtk_pdma_write(priv, RX_CRX_IDX_REG(0), idx);
	priv->rx_dma_owner_idx0 = (priv->rx_dma_owner_idx0 + 1) % NUM_RX_DESC;

	return 0;
}

static int mtk_eth_setup_rings_base(struct mtk_eth_priv *priv)
{
	uintptr_t rings_pa = DMA_BUF_ADDR, rings_va = rings_pa;
	uint32_t tx_ring_size, rx_ring_size;
	int ret = 0;

	tx_ring_size = TXD_SIZE * NUM_TX_DESC;
	tx_ring_size = round_up(tx_ring_size, ARCH_DMA_MINALIGN);

	rx_ring_size = RXD_SIZE * NUM_RX_DESC;
	rx_ring_size = round_up(rx_ring_size, ARCH_DMA_MINALIGN);

#ifdef DMA_BUF_REMAP
	ret = mmap_add_dynamic_region(rings_pa, rings_va, TX_RX_RING_SIZE,
				      MT_DEVICE | MT_RW);
	if (ret) {
		ERROR("mtk-eth: Map DMA ring pool failed with %d\n", ret);
		return -EFAULT;
	}
#endif

	priv->tx_ring_noc = (void *)rings_va;
	priv->rx_ring_noc = (void *)rings_va + tx_ring_size;

	priv->tx_ring_pa = rings_pa;
	priv->rx_ring_pa = rings_pa + tx_ring_size;

	return ret;
}

static int mtk_eth_setup_packet_base(struct mtk_eth_priv *priv)
{
	uintptr_t pkt_pa = DMA_BUF_ADDR + TX_RX_RING_SIZE, pkt_va = pkt_pa;
	int ret = 0;

#ifdef DMA_BUF_REMAP
	ret = mmap_add_dynamic_region(pkt_pa, pkt_va, ALIGNED_PKT_BUF_SIZE,
				      MT_MEMORY | MT_RW);
	if (ret) {
		ERROR("mtk-eth: Map DMA packet pool failed with %d\n", ret);
		return -EFAULT;
	}
#endif

	priv->pkt_pool_pa = pkt_pa;
	priv->pkt_pool_va = pkt_va;

	return ret;
}

static void mtk_eth_reset(struct mtk_eth_priv *priv)
{
	uint32_t mask = ETHSYS_RST_FE | ETHSYS_RST_GMAC;

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2) ||
	    MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		mtk_ethsys_rmw(priv, ETHSYS_FE_RST_CHK_IDLE_EN, 0xffffffff, 0);

	/* PPE */
	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2)) {
		mask |= ETHSYS_RST_PPE0_V2;
		if (MTK_HAS_CAPS(SOC_CAPS, MTK_RSTCTRL_PPE1))
			mask |= ETHSYS_RST_PPE1_V2;
	} else if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3)) {
		mask |= ETHSYS_RST_PPE0_V3;
		if (MTK_HAS_CAPS(SOC_CAPS, MTK_RSTCTRL_PPE1))
			mask |= ETHSYS_RST_PPE1_V3;
		if (MTK_HAS_CAPS(SOC_CAPS, MTK_RSTCTRL_PPE2))
			mask |= ETHSYS_RST_PPE2_V3;
	} else {
		mask |= ETHSYS_RST_PPE0_V1;
	}

	mtk_ethsys_rmw(priv, ETHSYS_RSTCTL_REG, 0, mask);
	udelay(1000);
	mtk_ethsys_rmw(priv, ETHSYS_RSTCTL_REG, mask, 0);
	mdelay(10);

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V2))
		mtk_ethsys_rmw(priv, ETHSYS_FE_RST_CHK_IDLE_EN, 0xffffffff, 0x3ffffff);
	else if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		mtk_ethsys_rmw(priv, ETHSYS_FE_RST_CHK_IDLE_EN, 0xffffffff, 0x6F8FF);
}

static int mtk_eth_probe(struct mtk_eth_priv *priv)
{
	/* Prepare for tx/rx rings */
	mtk_eth_setup_rings_base(priv);
	mtk_eth_setup_packet_base(priv);

	mtk_pll_eth_init();

	mtk_eth_reset(priv);

	mtk_ethsys_rmw(priv, ETHSYS_CLKCFG1, 0, 0xffffffff);

	/* Set MAC mode */
#if GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII
	mtk_xmac_init(priv);
#else
	mtk_mac_init(priv);
#endif

#if defined(PHY_MODE)
	/* Probe phy if switch is not specified */
	return mtk_phy_probe(dev);
#else
	/* Initialize switch */
	return mt753x_switch_init(priv);
#endif
}

int mtk_mdio_read(uint8_t addr, int devad, uint16_t reg)
{
	struct mtk_eth_priv *priv = &_eth_priv;

	if (devad < 0)
		return mtk_mii_read(priv, addr, reg);
	else
		return mtk_mmd_read(priv, addr, devad, reg);
}

int mtk_mdio_write(uint8_t addr, int devad, uint16_t reg, uint16_t val)
{
	struct mtk_eth_priv *priv = &_eth_priv;

	if (devad < 0)
		return mtk_mii_write(priv, addr, reg, val);
	else
		return mtk_mmd_write(priv, addr, devad, reg, val);
}

int mtk_eth_wait_connection_ready(uint32_t timeout_ms)
{
	struct mtk_eth_priv *priv = &_eth_priv;

#if defined(PHY_MODE)
	return -ENOTSUP;
#else
	return mt753x_switch_wait_port_ready(priv, timeout_ms);
#endif
}

void mtk_eth_cleanup(void)
{
#ifdef DMA_BUF_REMAP
	struct mtk_eth_priv *priv = &_eth_priv;
#endif

	/* Stop possibly started DMA */
	mtk_eth_stop();

#ifdef DMA_BUF_REMAP
	/* Unmap DMA ring pool */
	mmap_remove_dynamic_region((uintptr_t)priv->tx_ring_noc,
				   TX_RX_RING_SIZE);
	mmap_remove_dynamic_region(priv->pkt_pool_va, ALIGNED_PKT_BUF_SIZE);
#endif

	eth_ready = false;
}

int mtk_eth_init(void)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	int ret;

	if (eth_ready)
		return -EBUSY;

	memset(&_eth_priv, 0, sizeof(_eth_priv));

	ret = mtk_eth_probe(priv);
	if (!ret)
		eth_ready = true;

	return ret;
}
