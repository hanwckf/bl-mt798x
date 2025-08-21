/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 * Author: SkyLake.Huang <skylake.huang@mediatek.com>
 */

#ifndef _MTK_ETH_PHY_LIB_H_
#define _MTK_ETH_PHY_LIB_H_

#include <stdbool.h>
#include <common/debug.h>
#include "mtk_eth.h"
#include "phy_internal.h"

#define MTK_EXT_PAGE_ACCESS			0x1f
#define MTK_PHY_PAGE_STANDARD			0x0000
#define MTK_PHY_PAGE_EXTENDED_1			0x0001
#define MTK_PHY_AUX_CTRL_AND_STATUS		0x14
/* suprv_media_select_RefClk */
#define   MTK_PHY_LP_DETECTED_MASK		GENMASK(7, 6)
#define   MTK_PHY_ENABLE_DOWNSHIFT		BIT(4)

#define MTK_PHY_PAGE_EXTENDED_52B5		0x52b5

/* Registers on Token Ring debug nodes */
/* ch_addr = 0x0, node_addr = 0xf, data_addr = 0x2 */
#define   AN_STATE_MASK				GENMASK(22, 19)
#define   AN_STATE_SHIFT			19
#define   AN_STATE_TX_DISABLE			1

/* ch_addr = 0x0, node_addr = 0xf, data_addr = 0x3c */
#define AN_NEW_LP_CNT_LIMIT_MASK		GENMASK(23, 20)
#define AUTO_NP_10XEN				BIT(6)

/* Registers on MDIO_MMD_VEND1 */
#define MTK_PHY_LINK_STATUS_MISC		(0xa2)
#define   MTK_PHY_FINAL_SPEED_1000		BIT(3)

/* Registers on MDIO_MMD_VEND2 */
#define MTK_PHY_LED0_ON_CTRL			0x24
#define MTK_PHY_LED1_ON_CTRL			0x26
#define   MTK_GPHY_LED_ON_MASK			GENMASK(6, 0)
#define   MTK_2P5GPHY_LED_ON_MASK		GENMASK(7, 0)
#define   MTK_PHY_LED_ON_LINK1000		BIT(0)
#define   MTK_PHY_LED_ON_LINK100		BIT(1)
#define   MTK_PHY_LED_ON_LINK10			BIT(2)
#define   MTK_PHY_LED_ON_LINKDOWN		BIT(3)
#define   MTK_PHY_LED_ON_FDX			BIT(4) /* Full duplex */
#define   MTK_PHY_LED_ON_HDX			BIT(5) /* Half duplex */
#define   MTK_PHY_LED_ON_FORCE_ON		BIT(6)
#define   MTK_PHY_LED_ON_LINK2500		BIT(7)
#define   MTK_PHY_LED_ON_POLARITY		BIT(14)
#define   MTK_PHY_LED_ON_ENABLE			BIT(15)

#define MTK_PHY_LED0_BLINK_CTRL			0x25
#define MTK_PHY_LED1_BLINK_CTRL			0x27
#define   MTK_PHY_LED_BLINK_1000TX		BIT(0)
#define   MTK_PHY_LED_BLINK_1000RX		BIT(1)
#define   MTK_PHY_LED_BLINK_100TX		BIT(2)
#define   MTK_PHY_LED_BLINK_100RX		BIT(3)
#define   MTK_PHY_LED_BLINK_10TX		BIT(4)
#define   MTK_PHY_LED_BLINK_10RX		BIT(5)
#define   MTK_PHY_LED_BLINK_COLLISION		BIT(6)
#define   MTK_PHY_LED_BLINK_RX_CRC_ERR		BIT(7)
#define   MTK_PHY_LED_BLINK_RX_IDLE_ERR		BIT(8)
#define   MTK_PHY_LED_BLINK_FORCE_BLINK		BIT(9)
#define   MTK_PHY_LED_BLINK_2500TX		BIT(10)
#define   MTK_PHY_LED_BLINK_2500RX		BIT(11)

#define MTK_GPHY_LED_ON_SET			(MTK_PHY_LED_ON_LINK1000 | \
						 MTK_PHY_LED_ON_LINK100 | \
						 MTK_PHY_LED_ON_LINK10)
#define MTK_GPHY_LED_RX_BLINK_SET		(MTK_PHY_LED_BLINK_1000RX | \
						 MTK_PHY_LED_BLINK_100RX | \
						 MTK_PHY_LED_BLINK_10RX)
#define MTK_GPHY_LED_TX_BLINK_SET		(MTK_PHY_LED_BLINK_1000RX | \
						 MTK_PHY_LED_BLINK_100RX | \
						 MTK_PHY_LED_BLINK_10RX)

#define MTK_2P5GPHY_LED_ON_SET			(MTK_PHY_LED_ON_LINK2500 | \
						 MTK_GPHY_LED_ON_SET)
#define MTK_2P5GPHY_LED_RX_BLINK_SET		(MTK_PHY_LED_BLINK_2500RX | \
						 MTK_GPHY_LED_RX_BLINK_SET)
#define MTK_2P5GPHY_LED_TX_BLINK_SET		(MTK_PHY_LED_BLINK_2500RX | \
						 MTK_GPHY_LED_TX_BLINK_SET)

#define MTK_PHY_LED_STATE_FORCE_ON		0
#define MTK_PHY_LED_STATE_FORCE_BLINK		1
#define MTK_PHY_LED_STATE_NETDEV		2

static inline void mtk_phy_select_page(uint8_t phy, int page)
{
	mtk_mdio_write(phy, MDIO_DEVAD_NONE, MTK_EXT_PAGE_ACCESS, page);
}

static inline void mtk_phy_restore_page(uint8_t phy)
{
	mtk_mdio_write(phy, MDIO_DEVAD_NONE, MTK_EXT_PAGE_ACCESS,
		       MTK_PHY_PAGE_STANDARD);
}

/* Difference between functions with mtk_tr* and __mtk_tr* prefixes is
 * mtk_tr* functions: wrapped by page switching operations
 * __mtk_tr* functions: no page switching operations
 */
static inline void __mtk_tr_access(uint8_t phy, bool read, uint8_t ch_addr,
				   uint8_t node_addr, uint8_t data_addr)
{
	uint16_t tr_cmd = BIT(15); /* bit 14 & 0 are reserved */

	if (read)
		tr_cmd |= BIT(13);

	tr_cmd |= (((ch_addr & 0x3) << 11) |
		   ((node_addr & 0xf) << 7) |
		   ((data_addr & 0x3f) << 1));
	VERBOSE("tr_cmd: 0x%x\n", tr_cmd);
	mtk_mdio_write(phy, MDIO_DEVAD_NONE, 0x10, tr_cmd);
}

static inline void __mtk_tr_read(uint8_t phy, uint8_t ch_addr,
				 uint8_t node_addr, uint8_t data_addr,
				 uint16_t *tr_high, uint16_t *tr_low)
{
	__mtk_tr_access(phy, true, ch_addr, node_addr, data_addr);
	*tr_low = mtk_mdio_read(phy, MDIO_DEVAD_NONE, 0x11);
	*tr_high = mtk_mdio_read(phy, MDIO_DEVAD_NONE, 0x12);
	VERBOSE("tr_high read: 0x%x, tr_low read: 0x%x\n", *tr_high, *tr_low);
}

static inline uint32_t mtk_tr_read(uint8_t phy, uint8_t ch_addr,
				   uint8_t node_addr, uint8_t data_addr)
{
	uint16_t tr_high;
	uint16_t tr_low;

	mtk_phy_select_page(phy, MTK_PHY_PAGE_EXTENDED_52B5);
	__mtk_tr_read(phy, ch_addr, node_addr, data_addr, &tr_high, &tr_low);
	mtk_phy_restore_page(phy);

	return (tr_high << 16) | tr_low;
}

static inline void __mtk_tr_write(uint8_t phy, uint8_t ch_addr,
				  uint8_t node_addr, uint8_t data_addr,
				  uint32_t tr_data)
{
	mtk_mdio_write(phy, MDIO_DEVAD_NONE, 0x11, tr_data & 0xffff);
	mtk_mdio_write(phy, MDIO_DEVAD_NONE, 0x12, tr_data >> 16);
	VERBOSE("tr_high write: 0x%x, tr_low write: 0x%x\n",
		tr_data >> 16, tr_data & 0xffff);
	__mtk_tr_access(phy, false, ch_addr, node_addr, data_addr);
}

static inline void __mtk_tr_modify(uint8_t phy, uint8_t ch_addr,
				   uint8_t node_addr, uint8_t data_addr,
				   uint32_t mask, uint32_t set)
{
	uint32_t tr_data;
	uint16_t tr_high;
	uint16_t tr_low;

	__mtk_tr_read(phy, ch_addr, node_addr, data_addr, &tr_high, &tr_low);
	tr_data = (tr_high << 16) | tr_low;
	tr_data = (tr_data & ~mask) | set;
	__mtk_tr_write(phy, ch_addr, node_addr, data_addr, tr_data);
}

static inline void mtk_tr_modify(uint8_t phy, uint8_t ch_addr,
				 uint8_t node_addr, uint8_t data_addr,
				 uint32_t mask, uint32_t set)
{
	mtk_phy_select_page(phy, MTK_PHY_PAGE_EXTENDED_52B5);
	__mtk_tr_modify(phy, ch_addr, node_addr, data_addr, mask, set);
	mtk_phy_restore_page(phy);
}

static inline void __mtk_tr_set_bits(uint8_t phy, uint8_t ch_addr,
				     uint8_t node_addr, uint8_t data_addr,
				     uint32_t set)
{
	__mtk_tr_modify(phy, ch_addr, node_addr, data_addr, 0, set);
}

static inline void __mtk_tr_clr_bits(uint8_t phy, uint8_t ch_addr,
				     uint8_t node_addr, uint8_t data_addr,
				     uint32_t clr)
{
	__mtk_tr_modify(phy, ch_addr, node_addr, data_addr, clr, 0);
}

static inline int mtk_mdio_set_bits(uint8_t phy, int devad, uint32_t regnum,
				    uint16_t bits)
{
	int value, ret;

	value = mtk_mdio_read(phy, devad, regnum);
	if (value < 0)
		return value;

	value |= bits;

	ret = mtk_mdio_write(phy, devad, regnum, value);
	if (ret < 0)
		return ret;

	return 0;
}

static inline int mtk_mdio_clr_bits(uint8_t phy, int devad, uint32_t regnum,
				    uint16_t bits)
{
	int value, ret;

	value = mtk_mdio_read(phy, devad, regnum);
	if (value < 0)
		return value;

	value &= ~bits;

	ret = mtk_mdio_write(phy, devad, regnum, value);
	if (ret < 0)
		return ret;

	return 0;
}

static inline int mtk_mdio_clrset_bits(uint8_t phy, int devad, uint32_t regnum,
				       uint16_t clr, uint16_t set)
{
	int value, new, ret;

	value = mtk_mdio_read(phy, devad, regnum);
	if (value < 0)
		return value;

	new = (value & (~clr)) | set;
	if (value == new)
		return 0;

	ret = mtk_mdio_write(phy, devad, regnum, new);
	if (ret < 0)
		return ret;

	return 0;
}

#endif /* _MTK_ETH_PHY_LIB_H_ */
