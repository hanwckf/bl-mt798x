/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_ETH_H_
#define _MTK_ETH_H_

#include <stdint.h>
#include <stdbool.h>
#include <lib/utils_def.h>
#include "phy.h"

enum mkt_eth_capabilities {
	MTK_TRGMII_BIT,
	MTK_TRGMII_MT7621_CLK_BIT,
	MTK_U3_COPHY_V2_BIT,
	MTK_INFRA_BIT,
	MTK_NETSYS_V2_BIT,
	MTK_NETSYS_V3_BIT,
	MTK_RSTCTRL_PPE1_BIT,
	MTK_RSTCTRL_PPE2_BIT,

	/* PATH BITS */
	MTK_ETH_PATH_GMAC1_TRGMII_BIT,
	MTK_ETH_PATH_GMAC2_SGMII_BIT,
	MTK_ETH_PATH_MT7622_SGMII_BIT,
	MTK_ETH_PATH_MT7629_GMAC2_BIT,
};

#define MTK_TRGMII			BIT(MTK_TRGMII_BIT)
#define MTK_TRGMII_MT7621_CLK		BIT(MTK_TRGMII_MT7621_CLK_BIT)
#define MTK_U3_COPHY_V2			BIT(MTK_U3_COPHY_V2_BIT)
#define MTK_INFRA			BIT(MTK_INFRA_BIT)
#define MTK_NETSYS_V2			BIT(MTK_NETSYS_V2_BIT)
#define MTK_NETSYS_V3			BIT(MTK_NETSYS_V3_BIT)
#define MTK_RSTCTRL_PPE1		BIT(MTK_RSTCTRL_PPE1_BIT)
#define MTK_RSTCTRL_PPE2		BIT(MTK_RSTCTRL_PPE2_BIT)

/* Supported path present on SoCs */
#define MTK_ETH_PATH_GMAC1_TRGMII	BIT(MTK_ETH_PATH_GMAC1_TRGMII_BIT)
#define MTK_ETH_PATH_GMAC2_SGMII	BIT(MTK_ETH_PATH_GMAC2_SGMII_BIT)
#define MTK_ETH_PATH_MT7622_SGMII	BIT(MTK_ETH_PATH_MT7622_SGMII_BIT)
#define MTK_ETH_PATH_MT7629_GMAC2	BIT(MTK_ETH_PATH_MT7629_GMAC2_BIT)

#define MTK_GMAC1_TRGMII	(MTK_ETH_PATH_GMAC1_TRGMII | MTK_TRGMII)

#define MTK_GMAC2_U3_QPHY	(MTK_ETH_PATH_GMAC2_SGMII | MTK_U3_COPHY_V2 | MTK_INFRA)

/* Frame Engine Register Bases */
#define PDMA_V1_BASE			0x0800
#define PDMA_V2_BASE			0x6000
#define PDMA_V3_BASE			0x6800

int mtk_eth_start(void);
void mtk_eth_stop(void);
void mtk_eth_write_hwaddr(const uint8_t *addr);
int mtk_eth_send(const void *packet, uint32_t length);
int mtk_eth_recv(void **packetp);
int mtk_eth_free_pkt(void *packet);

int mtk_mdio_read(uint8_t addr, int devad, uint16_t reg);
int mtk_mdio_write(uint8_t addr, int devad, uint16_t reg, uint16_t val);

int mtk_eth_wait_connection_ready(uint32_t timeout_ms);

void mtk_eth_cleanup(void);
int mtk_eth_init(void);

#endif /* _MTK_ETH_H_ */
