/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: SkyLake.Huang <skylake.huang@mediatek.com>
 */

#ifndef _PLAT_I2P5GE_LOAD_FW_H_
#define _PLAT_I2P5GE_LOAD_FW_H_

#include <endian.h>
#include <lib/utils_def.h>

static int i2p5ge_phy_plat_init(uint8_t phy)
{
	/* Load FW here if needed */

	mtk_mdio_clr_bits(phy, MDIO_MMD_VEND2, MTK_PHY_LED0_ON_CTRL,
			  MTK_PHY_LED_ON_POLARITY);

	return 0;
}

#endif /* _PLAT_I2P5GE_LOAD_FW_H_ */
