/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef _MTK_ETH_PHY_H_
#define _MTK_ETH_PHY_H_

#include <stdint.h>

#define PHY_INTERFACE_MODE_RGMII	0
#define PHY_INTERFACE_MODE_SGMII	1
#define PHY_INTERFACE_MODE_2500BASEX	2
#define PHY_INTERFACE_MODE_USXGMII	3
#define PHY_INTERFACE_MODE_XGMII	4
#define PHY_INTERFACE_MODE_10GBASER	5

#define SPEED_10			10
#define SPEED_100			100
#define SPEED_1000			1000
#define SPEED_2500			2500
#define SPEED_10000			10000

#define DUPLEX_HALF			0x00
#define DUPLEX_FULL			0x01

#define MDIO_DEVAD_NONE			(-1)

#define MII_BMCR			0x00	/* Basic mode control register */
#define BMCR_SPEED1000			0x0040	/* MSB of Speed (1000)         */
#define BMCR_FULLDPLX			0x0100	/* Full duplex                 */
#define BMCR_PDOWN			0x0800	/* Enable low power state      */
#define BMCR_SPEED100			0x2000	/* Select 100Mbps              */
#define BMCR_RESET			0x8000	/* Reset to default state      */

#define MII_BMSR			0x01	/* Basic mode status register  */
#define BMSR_LSTATUS			0x0004	/* Link status                 */
#define BMSR_ANEGCOMPLETE		0x0020	/* Auto-negotiation complete   */

#define MII_ADVERTISE			0x04	/* Advertisement control reg   */

#define MII_LPA				0x05	/* Link partner ability reg    */
#define LPA_10HALF			0x0020	/* Can do 10mbps half-duplex   */
#define LPA_10FULL			0x0040	/* Can do 10mbps full-duplex   */
#define LPA_100HALF			0x0080	/* Can do 100mbps half-duplex  */
#define LPA_100FULL			0x0100	/* Can do 100mbps full-duplex  */

#define MII_CTRL1000			0x09	/* 1000BASE-T control          */

#define MII_STAT1000			0x0a	/* 1000BASE-T status           */
#define PHY_1000BTSR_1000FD		0x0800
#define PHY_1000BTSR_1000HD		0x0400

#define MDIO_MMD_VEND1			30	/* Vendor specific 1 */
#define MDIO_MMD_VEND2			31	/* Vendor specific 2 */

/* phy driver declarations */
struct genphy_status {
	uint32_t link;
	uint32_t speed;
	uint32_t duplex;
};

int eth_phy_init(uint8_t phy);
int eth_phy_start(uint8_t phy);
int eth_phy_get_status(uint8_t phy, struct genphy_status *status);
int eth_phy_wait_ready(uint8_t phy, uint32_t timeout_ms);

#endif /* _MTK_ETH_PHY_H_ */
