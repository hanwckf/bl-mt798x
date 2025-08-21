// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <errno.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include "mtk_eth.h"
#include "phy_internal.h"

void gen_phy_get_status(uint8_t phy, struct genphy_status *status)
{
	uint16_t bmsr, sts;

	bmsr = mtk_mdio_read(phy, MDIO_DEVAD_NONE, MII_BMSR);
	if (!(bmsr & BMSR_LSTATUS)) {
		status->link = false;
		status->speed = SPEED_10;
		status->duplex = DUPLEX_HALF;
	} else {
		status->link = true;

		sts = mtk_mdio_read(phy, MDIO_DEVAD_NONE, MII_STAT1000);
		sts &= mtk_mdio_read(phy, MDIO_DEVAD_NONE, MII_CTRL1000) << 2;

		if (sts & (PHY_1000BTSR_1000FD | PHY_1000BTSR_1000HD)) {
			status->speed = SPEED_1000;

			if (sts & PHY_1000BTSR_1000FD)
				status->duplex = DUPLEX_FULL;
		} else {
			sts = mtk_mdio_read(phy, MDIO_DEVAD_NONE, MII_ADVERTISE);
			sts &= mtk_mdio_read(phy, MDIO_DEVAD_NONE, MII_LPA);

			if (sts & (LPA_100FULL | LPA_100HALF)) {
				status->speed = SPEED_100;

				if (sts & LPA_100FULL)
					status->duplex = DUPLEX_FULL;
			} else {
				status->speed = SPEED_10;

				if (sts & LPA_10FULL)
					status->duplex = DUPLEX_FULL;
				else
					status->duplex = DUPLEX_HALF;
			}
		}
	}
}

int gen_phy_wait_ready(uint8_t phy, uint32_t timeout_ms)
{
	uint64_t tmo = timeout_init_us((uint64_t)timeout_ms * 1000);
	bool print_msg = true;
	uint16_t sts;

	while (timeout_ms == UINT32_MAX || !timeout_elapsed(tmo)) {
		sts = mtk_mdio_read(phy, MDIO_DEVAD_NONE, MII_BMSR);
		if (sts & BMSR_ANEGCOMPLETE) {
			if (sts & BMSR_LSTATUS)
				return 0;
		}

		if (print_msg) {
			print_msg = false;
			NOTICE("Waiting for PHY auto negotiation to complete\n");
		}

		mdelay(100);
	}

	return -ETIMEDOUT;
}
