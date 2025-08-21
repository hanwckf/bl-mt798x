// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <stdint.h>
#include <common/debug.h>
#include <lib/xlat_tables/xlat_tables_v2.h>
#include <plat_eth_def.h>
#include <pll.h>
#include "mtk_eth_internal.h"
#include "phy_internal.h"

#define NUM_TX_DESC		8
#define NUM_RX_DESC		8
#define PKTSIZE_ALIGN		1536
#define ARCH_DMA_MINALIGN	64
#define TX_TOTAL_BUF_SIZE	(NUM_TX_DESC * PKTSIZE_ALIGN)
#define RX_TOTAL_BUF_SIZE	(NUM_RX_DESC * PKTSIZE_ALIGN)
#define PKT_TOTAL_BUF_SIZE	(TX_TOTAL_BUF_SIZE + RX_TOTAL_BUF_SIZE)
#define TX_RX_RING_SIZE		PAGE_SIZE
#define ALIGNED_PKT_BUF_SIZE	round_up(PKT_TOTAL_BUF_SIZE, PAGE_SIZE)

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

#ifndef MTK_ETH_PHY_ADDR
#define MTK_ETH_PHY_ADDR	8	/* dummy */
#endif

#ifndef XFI_PLL_BASE
#define XFI_PLL_BASE		0	/* dummy */
#endif

#ifndef XFI_PEXTP_BASE
#define XFI_PEXTP_BASE		0	/* dummy */
#endif

#ifndef USXGMII_BASE
#define USXGMII_BASE		0	/* dummy */
#endif

#ifndef GMAC_FORCE_SPEED
#define GMAC_FORCE_SPEED	0
#endif

#ifndef GMAC_FORCE_FULL_DUPLEX
#define GMAC_FORCE_FULL_DUPLEX	0
#endif

static struct mtk_eth_priv _eth_priv;
static bool eth_ready;

static void mtk_pdma_write(struct mtk_eth_priv *priv, u32 reg, u32 val)
{
	mmio_write_32(FE_BASE + PDMA_BASE + reg, val);
}

static void mtk_pdma_rmw(struct mtk_eth_priv *priv, u32 reg, u32 clr, u32 set)
{
	mmio_clrsetbits_32(FE_BASE + PDMA_BASE + reg, clr, set);
}

static void mtk_gdma_write(struct mtk_eth_priv *priv, int no, u32 reg, u32 val)
{
	u32 gdma_base;

	if (no == 2)
		gdma_base = GDMA3_BASE;
	else if (no == 1)
		gdma_base = GDMA2_BASE;
	else
		gdma_base = GDMA1_BASE;

	mmio_write_32(FE_BASE + gdma_base + reg, val);
}

static u32 mtk_gmac_read(struct mtk_eth_priv *priv, u32 reg)
{
	return mmio_read_32(FE_BASE + GMAC_BASE + reg);
}

static void mtk_gmac_write(struct mtk_eth_priv *priv, u32 reg, u32 val)
{
	mmio_write_32(FE_BASE + GMAC_BASE + reg, val);
}

static void __maybe_unused mtk_infra_rmw(struct mtk_eth_priv *priv, u32 reg,
			  u32 clr, u32 set)
{
	mmio_clrsetbits_32(TOP_MISC_BASE + reg, clr, set);
}

/* Direct MDIO clause 22/45 access via SoC */
static int mtk_mii_rw(struct mtk_eth_priv *priv, u8 phy, u8 reg, u16 data,
		      u32 cmd, u32 st)
{
	int ret;
	u32 val;

	val = (st << MDIO_ST_S) |
	      ((cmd << MDIO_CMD_S) & MDIO_CMD_M) |
	      (((u32)phy << MDIO_PHY_ADDR_S) & MDIO_PHY_ADDR_M) |
	      (((u32)reg << MDIO_REG_ADDR_S) & MDIO_REG_ADDR_M);

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
int mtk_mii_read(struct mtk_eth_priv *priv, u8 phy, u8 reg)
{
	return mtk_mii_rw(priv, phy, reg, 0, MDIO_CMD_READ, MDIO_ST_C22);
}

/* Direct MDIO clause 22 write via SoC */
int mtk_mii_write(struct mtk_eth_priv *priv, u8 phy, u8 reg, u16 data)
{
	return mtk_mii_rw(priv, phy, reg, data, MDIO_CMD_WRITE, MDIO_ST_C22);
}

/* Direct MDIO clause 45 read via SoC */
int mtk_mmd_read(struct mtk_eth_priv *priv, u8 addr, u8 devad, u16 reg)
{
	int ret;

	ret = mtk_mii_rw(priv, addr, devad, reg, MDIO_CMD_ADDR, MDIO_ST_C45);
	if (ret)
		return ret;

	return mtk_mii_rw(priv, addr, devad, 0, MDIO_CMD_READ_C45,
			  MDIO_ST_C45);
}

/* Direct MDIO clause 45 write via SoC */
int mtk_mmd_write(struct mtk_eth_priv *priv, u8 addr, u8 devad, u16 reg,
		  u16 val)
{
	int ret;

	ret = mtk_mii_rw(priv, addr, devad, reg, MDIO_CMD_ADDR, MDIO_ST_C45);
	if (ret)
		return ret;

	return mtk_mii_rw(priv, addr, devad, val, MDIO_CMD_WRITE,
			  MDIO_ST_C45);
}

/* Indirect MDIO clause 45 read via MII registers */
int mtk_mmd_ind_read(struct mtk_eth_priv *priv, u8 addr, u8 devad, u16 reg)
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
int mtk_mmd_ind_write(struct mtk_eth_priv *priv, u8 addr, u8 devad, u16 reg,
		      u16 val)
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

#ifdef SWITCH_MT7531
extern const struct mtk_eth_switch switch_mt7531;
#endif

#ifdef SWITCH_MT7988
extern const struct mtk_eth_switch switch_mt7988;
#endif

#ifdef SWITCH_AN8855
extern const struct mtk_eth_switch switch_an8855;
#endif

static const struct mtk_eth_switch *const mtk_switch[] = {
#ifdef SWITCH_MT7531
	&switch_mt7531,
#endif
#ifdef SWITCH_MT7988
	&switch_mt7988,
#endif
#ifdef SWITCH_AN8855
	&switch_an8855,
#endif
};

static int mtk_switch_init(struct mtk_eth_priv *priv)
{
	const struct mtk_eth_switch *tmp, *swdrv = NULL;
	const u32 nsw = ARRAY_SIZE(mtk_switch);
	u32 reset_wait_time = 500;
	int ret;
	u32 i;

	if (nsw == 1) {
		swdrv = mtk_switch[0];
		reset_wait_time = swdrv->reset_wait_time;
	}

	/* Global reset switch */
#ifdef SWITCH_RESET_MCM
	mmio_setbits_32(SWITCH_RESET_MCM_REG, BIT(SWITCH_RESET_MCM_BIT));
	mdelay(10);
	mmio_clrbits_32(SWITCH_RESET_MCM_REG, BIT(SWITCH_RESET_MCM_BIT));
	mdelay(reset_wait_time);
#elif defined(SWITCH_RESET_GPIO)
	mtk_plat_switch_reset(true);	/* Provided in plat_eth_def.h */
	mdelay(10);
	mtk_plat_switch_reset(false);
	mdelay(reset_wait_time);
#endif

	if (!swdrv) {
		for (i = 0; i < nsw; i++) {
			tmp = mtk_switch[i];

			if (!tmp->detect)
				continue;

			ret = tmp->detect(priv);
			if (!ret) {
				swdrv = tmp;
				break;
			}
		}

		if (!swdrv) {
			ERROR("Unable to detect switch\n");
			return -ENODEV;
		}
	} else {
		if (swdrv->detect) {
			ret = swdrv->detect(priv);
			if (ret) {
				ERROR("Switch probing failed\n");
				return -ENODEV;
			}
		}
	}

	NOTICE("ETH: %s\n", swdrv->desc);

	ret = swdrv->setup(priv);
	if (ret)
		return ret;

	priv->swdrv = swdrv;

	return 0;
}

static void mtk_xphy_link_adjust(struct mtk_eth_priv *priv,
				 struct genphy_status *status)
{
	u32 mcr;

	mcr = mtk_gmac_read(priv, XGMAC_PORT_MCR(GMAC_ID));
	mcr &= ~(XGMAC_FORCE_TX_FC | XGMAC_FORCE_RX_FC);
	mcr &= ~(XGMAC_TRX_DISABLE);
	mtk_gmac_write(priv, XGMAC_PORT_MCR(GMAC_ID), mcr);
}

static void mtk_phy_link_adjust(struct mtk_eth_priv *priv,
				struct genphy_status *status)
{
	u32 mcr;

	mcr = (IPG_96BIT_WITH_SHORT_IPG << IPG_CFG_S) |
	      (MAC_RX_PKT_LEN_1536 << MAC_RX_PKT_LEN_S) |
	      MAC_MODE | FORCE_MODE |
	      MAC_TX_EN | MAC_RX_EN |
	      DEL_RXFIFO_CLR |
	      BKOFF_EN | BACKPR_EN;

	switch (status->speed) {
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

	if (status->link)
		mcr |= FORCE_LINK;

	if (status->duplex)
		mcr |= FORCE_DPX;

	mtk_gmac_write(priv, GMAC_PORT_MCR(GMAC_ID), mcr);
}

static int mtk_phy_start(struct mtk_eth_priv *priv)
{
	struct genphy_status status;
	int ret;

	ret = eth_phy_start(MTK_ETH_PHY_ADDR);
	if (ret) {
		ERROR("error: Could not initialize PHY\n");
		return ret;
	}

	ret = eth_phy_get_status(MTK_ETH_PHY_ADDR, &status);
	if (ret) {
		WARN("warning: port link down\n");
		return 0;
	}

	if (!GMAC_FORCE_MODE) {
		if ((GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII) ||
		    (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_10GBASER) ||
		    (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_XGMII))
			mtk_xphy_link_adjust(priv, &status);
		else
			mtk_phy_link_adjust(priv, &status);
	}

	VERBOSE("Speed: %d, %s duplex\n", status.speed,
		(status.duplex) ? "full" : "half");

	return 0;
}

static int mtk_phy_probe(struct mtk_eth_priv *priv)
{
	/* No need to probe PHY here. Do PHY initialization directly */
	eth_phy_init(MTK_ETH_PHY_ADDR);

	return 0;
}

static int mtk_phy_wait_ready(struct mtk_eth_priv *priv, u32 timeout_ms)
{
	return eth_phy_wait_ready(MTK_ETH_PHY_ADDR, timeout_ms);
}

static void mtk_sgmii_an_init(struct mtk_eth_priv *priv)
{
	/* Set SGMII GEN1 speed(1G) */
	mmio_clrbits_32(SGMII_BASE + ANA_RG3_OFFS, SGMSYS_SPEED_MASK);

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
	mmio_clrbits_32(SGMII_BASE + SGMSYS_QPHY_PWR_STATE_CTRL,
			SGMII_PHYA_PWD);
}

static void mtk_sgmii_force_init(struct mtk_eth_priv *priv)
{
	/* Set SGMII GEN2 speed(2.5G) */
	mmio_clrsetbits_32(SGMII_BASE + ANA_RG3_OFFS, SGMSYS_SPEED_MASK,
			   FIELD_PREP(SGMSYS_SPEED_MASK, SGMSYS_SPEED_2500));

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
	mmio_clrbits_32(SGMII_BASE + SGMSYS_QPHY_PWR_STATE_CTRL,
			SGMII_PHYA_PWD);
}

static void mtk_xfi_pll_enable(struct mtk_eth_priv *priv)
{
	/* Add software workaround for USXGMII PLL TCL issue */
	mmio_write_32(XFI_PLL_BASE + XFI_PLL_ANA_GLB8, RG_XFI_PLL_ANA_SWWA);
	mmio_setbits_32(XFI_PLL_BASE + XFI_PLL_DIG_GLB8, RG_XFI_PLL_EN);
}

static void mtk_usxgmii_reset(struct mtk_eth_priv *priv)
{
	if (GMAC_ID == 1) {
		mmio_write_32(TOPRGU_BASE + 0xFC, 0x0000A004);
		mmio_write_32(TOPRGU_BASE + 0x18, 0x88F0A004);
		mmio_write_32(TOPRGU_BASE + 0xFC, 0x00000000);
		mmio_write_32(TOPRGU_BASE + 0x18, 0x88F00000);
		mmio_write_32(TOPRGU_BASE + 0x18, 0x00F00000);
	} else if (GMAC_ID == 2) {
		mmio_write_32(TOPRGU_BASE + 0xFC, 0x00005002);
		mmio_write_32(TOPRGU_BASE + 0x18, 0x88F05002);
		mmio_write_32(TOPRGU_BASE + 0xFC, 0x00000000);
		mmio_write_32(TOPRGU_BASE + 0x18, 0x88F00000);
		mmio_write_32(TOPRGU_BASE + 0x18, 0x00F00000);
	}

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

static void mtk_usxgmii_setup_phya_force_10000(struct mtk_eth_priv *priv)
{
	mmio_write_32(USXGMII_BASE + 0x810, 0x000FFE6C);
	mmio_write_32(USXGMII_BASE + 0x818, 0x07B1EC7B);
	mmio_write_32(USXGMII_BASE + 0x80C, 0xB0000000);
	udelay(2);
	mmio_write_32(USXGMII_BASE + 0x80C, 0x90000000);
	udelay(2);

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
	mmio_write_32(XFI_PEXTP_BASE + 0x3048, 0x47684100);
	mmio_write_32(XFI_PEXTP_BASE + 0x3050, 0x00000000);
	mmio_write_32(XFI_PEXTP_BASE + 0x3054, 0x00000000);
	mmio_write_32(XFI_PEXTP_BASE + 0x306C, 0x00000F00);
	if (GMAC_ID == 2)
		mmio_write_32(XFI_PEXTP_BASE + 0xA008, 0x0007B400);
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
	mtk_xfi_pll_enable(priv);
	mtk_usxgmii_reset(priv);
	mtk_usxgmii_setup_phya_an_10000(priv);
}

static void mtk_10gbaser_init(struct mtk_eth_priv *priv)
{
	mtk_xfi_pll_enable(priv);
	mtk_usxgmii_reset(priv);
	mtk_usxgmii_setup_phya_force_10000(priv);
}

static void mtk_mac_init(struct mtk_eth_priv *priv)
{
	int i, sgmii_sel_mask = 0, ge_mode = 0;
	u32 mcr;

	if (MTK_HAS_CAPS(SOC_CAPS, MTK_ETH_PATH_MT7629_GMAC2)) {
		mtk_infra_rmw(priv, MT7629_INFRA_MISC2_REG,
			      INFRA_MISC2_BONDING_OPTION, GMAC_ID);
	}

	switch (GMAC_INTERFACE_MODE) {
	case PHY_INTERFACE_MODE_RGMII:
		ge_mode = GE_MODE_RGMII;
		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		if (MTK_HAS_CAPS(SOC_CAPS, MTK_GMAC2_U3_QPHY)) {
			mtk_infra_rmw(priv, USB_PHY_SWITCH_REG, QPHY_SEL_MASK,
				      SGMII_QPHY_SEL);
		}

		if (MTK_HAS_CAPS(SOC_CAPS, MTK_ETH_PATH_MT7622_SGMII))
			sgmii_sel_mask = SYSCFG1_SGMII_SEL_M;

		mtk_ethsys_rmw(priv, ETHSYS_SYSCFG1_REG, sgmii_sel_mask,
			       SYSCFG1_SGMII_SEL(GMAC_ID));

		if (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_SGMII)
			mtk_sgmii_an_init(priv);
		else
			mtk_sgmii_force_init(priv);

		ge_mode = GE_MODE_RGMII;
		break;
	default:
		break;
	}

	/* set the gmac to the right mode */
	mtk_ethsys_rmw(priv, ETHSYS_SYSCFG1_REG,
		       SYSCFG1_GE_MODE_M << SYSCFG1_GE_MODE_S(GMAC_ID),
		       ge_mode << SYSCFG1_GE_MODE_S(GMAC_ID));

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

static void mtk_xmac_init(struct mtk_eth_priv *priv)
{
	u32 force_link = 0;

	if (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII)
		mtk_usxgmii_an_init(priv);
	else if (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_10GBASER)
		mtk_10gbaser_init(priv);

	/* Set GMAC to the correct mode */
	mtk_ethsys_rmw(priv, ETHSYS_SYSCFG1_REG,
		       SYSCFG1_GE_MODE_M << SYSCFG1_GE_MODE_S(GMAC_ID), 0);

	if ((GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII ||
	     GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_10GBASER) &&
	    GMAC_ID == 1) {
		mtk_infra_rmw(priv, TOPMISC_NETSYS_PCS_MUX,
			      NETSYS_PCS_MUX_MASK, MUX_G2_USXGMII_SEL);
	}

	if (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_XGMII || GMAC_ID == 2)
		force_link = XGMAC_FORCE_LINK(GMAC_ID);

	mtk_gmac_rmw(priv, XGMAC_STS(GMAC_ID), XGMAC_FORCE_LINK(GMAC_ID),
		     force_link);

	/* Force GMAC link down */
	mtk_gmac_write(priv, GMAC_PORT_MCR(GMAC_ID), FORCE_MODE);
}

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

static void mtk_eth_mdc_init(struct mtk_eth_priv *priv)
{
#if MDC_SRC_CLK
	u32 divider;

	divider = min_t(u32, DIV_ROUND_UP(MDC_MAX_FREQ, MDC_SRC_CLK),
			MDC_MAX_DIVIDER);

	/* Configure MDC turbo mode */
	if (MTK_HAS_CAPS(SOC_CAPS, MTK_NETSYS_V3))
		mtk_gmac_rmw(priv, GMAC_MAC_MISC_REG, 0, MISC_MDC_TURBO);
	else
		mtk_gmac_rmw(priv, GMAC_PPSC_REG, 0, MISC_MDC_TURBO);

	/* Configure MDC divider */
	mtk_gmac_rmw(priv, GMAC_PPSC_REG, PHY_MDC_CFG,
		     FIELD_PREP(PHY_MDC_CFG, divider));
#endif
}

int mtk_eth_start(void)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	u32 is_mt7988 = 0;
	int i;

#ifdef SWITCH_MT7988
	is_mt7988 = 1;
#endif

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
		if (is_mt7988 && GMAC_ID == 0) {
			mtk_gdma_write(priv, GMAC_ID, GDMA_IG_CTRL_REG,
				       GDMA_BRIDGE_TO_CPU);

			mtk_gdma_write(priv, GMAC_ID, GDMA_EG_CTRL_REG,
				       GDMA_CPU_BRIDGE_EN);
		} else if ((GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII ||
			    GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_10GBASER ||
			    GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_XGMII) &&
			   GMAC_ID != 0) {
			mtk_gdma_write(priv, GMAC_ID, GDMA_EG_CTRL_REG,
				       GDMA_CPU_BRIDGE_EN);
		}
	}

	udelay(500);

	mtk_eth_fifo_init(priv);

	if (!PHY_MODE) {
		/* Enable communication with switch */
		if (priv->swdrv->mac_control)
			priv->swdrv->mac_control(priv, true);
	} else {
		/* Start PHY */
		mtk_phy_start(priv);
	}

	mtk_pdma_rmw(priv, PDMA_GLO_CFG_REG, 0,
		     TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN);
	udelay(500);

	return 0;
}

void mtk_eth_stop(void)
{
	struct mtk_eth_priv *priv = &_eth_priv;

	if (!PHY_MODE) {
		if (priv->swdrv->mac_control)
			priv->swdrv->mac_control(priv, false);
	}

	mtk_pdma_rmw(priv, PDMA_GLO_CFG_REG,
		     TX_WB_DDONE | RX_DMA_EN | TX_DMA_EN, 0);
	udelay(500);

	wait_for_bit_32(FE_BASE + PDMA_BASE + PDMA_GLO_CFG_REG,
			RX_DMA_BUSY | TX_DMA_BUSY, false, 5000);
}

void mtk_eth_write_hwaddr(const uint8_t *addr)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	u32 macaddr_lsb, macaddr_msb;
	const uint8_t *mac = addr;

	if (!mac)
		return;

	macaddr_msb = ((u32)mac[0] << 8) | (u32)mac[1];
	macaddr_lsb = ((u32)mac[2] << 24) | ((u32)mac[3] << 16) |
		      ((u32)mac[4] << 8) | (u32)mac[5];

	mtk_gdma_write(priv, GMAC_ID, GDMA_MAC_MSB_REG, macaddr_msb);
	mtk_gdma_write(priv, GMAC_ID, GDMA_MAC_LSB_REG, macaddr_lsb);
}

int mtk_eth_send(const void *packet, uint32_t length)
{
	struct mtk_eth_priv *priv = &_eth_priv;
	u32 idx = priv->tx_cpu_owner_idx0;
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
	u32 idx = priv->rx_dma_owner_idx0;
	struct mtk_rx_dma_v2 *rxd;
	void *pkt_base;
	u32 length;

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
	u32 idx = priv->rx_dma_owner_idx0;
	struct mtk_rx_dma_v2 *rxd;

	rxd = priv->rx_ring_noc + idx * RXD_SIZE;

	inv_dcache_range((uintptr_t)rxd->rxd1,
			 roundup(PKTSIZE_ALIGN, ARCH_DMA_MINALIGN));

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

	if (DMA_BUF_REMAP) {
		ret = mmap_add_dynamic_region(rings_pa, rings_va,
					      TX_RX_RING_SIZE,
					      MT_DEVICE | MT_RW);
		if (ret) {
			ERROR("mtk-eth: Map DMA ring pool failed with %d\n",
			      ret);
			return -EFAULT;
		}
	}

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

	if (DMA_BUF_REMAP) {
		ret = mmap_add_dynamic_region(pkt_pa, pkt_va,
					      ALIGNED_PKT_BUF_SIZE,
					      MT_MEMORY | MT_RW);
		if (ret) {
			ERROR("mtk-eth: Map DMA packet pool failed with %d\n",
			      ret);
			return -EFAULT;
		}
	}

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

	/* Set MDC divider */
	mtk_eth_mdc_init(priv);

	/* Set MAC mode */
	if ((GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_USXGMII) ||
	    (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_10GBASER) ||
	    (GMAC_INTERFACE_MODE == PHY_INTERFACE_MODE_XGMII))
		mtk_xmac_init(priv);
	else
		mtk_mac_init(priv);

	if (PHY_MODE) {
		/* Probe phy if switch is not specified */
		return mtk_phy_probe(priv);
	}

	/* Initialize switch */
	return mtk_switch_init(priv);
}

int mtk_eth_wait_connection_ready(uint32_t timeout_ms)
{
	struct mtk_eth_priv *priv = &_eth_priv;

	if (PHY_MODE)
		return mtk_phy_wait_ready(priv, timeout_ms);

	return priv->swdrv->wait_ready(priv, timeout_ms);
}

void mtk_eth_cleanup(void)
{
	struct mtk_eth_priv *priv = &_eth_priv;

	/* Stop possibly started DMA */
	mtk_eth_stop();

	if (!PHY_MODE) {
		if (priv->swdrv->cleanup)
			priv->swdrv->cleanup(priv);
	}

	if (DMA_BUF_REMAP) {
		/* Unmap DMA ring pool */
		mmap_remove_dynamic_region((uintptr_t)priv->tx_ring_noc,
					   TX_RX_RING_SIZE);
		mmap_remove_dynamic_region(priv->pkt_pool_va,
					   ALIGNED_PKT_BUF_SIZE);
	}

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
