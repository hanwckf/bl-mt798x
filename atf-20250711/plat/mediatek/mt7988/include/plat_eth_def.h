/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _PLAT_ETH_DEF_H_
#define _PLAT_ETH_DEF_H_

#include <reg_base.h>

/* SoC-specific definitions */
#define SGMII_BASE			SGMII_SBUS0_BASE
#define ETHSYS_BASE			NETSYS1_BASE
#define FE_BASE				(ETHSYS_BASE + 0x100000)

#define PDMA_BASE			PDMA_V3_BASE
#define ANA_RG3_OFFS			0x128
#define GDMA_COUNT			3

#define SWITCH_RESET_MCM
#define SWITCH_RESET_MCM_REG		(ETHSYS_BASE + 0x31008)
#define SWITCH_RESET_MCM_BIT		9

#define TXD_SIZE			32
#define RXD_SIZE			32

#define SOC_CAPS			(MTK_NETSYS_V3 | MTK_INFRA | \
					 MTK_RSTCTRL_PPE1 | MTK_RSTCTRL_PPE2)

#if defined(IMAGE_BL31)
#define DMA_BUF_ADDR			INTER_SRAM
#define DMA_BUF_REMAP			1
#elif defined(IMAGE_BL2)
#include <bl2_plat_setup.h>
#define DMA_BUF_ADDR			SCRATCH_BUF_OFFSET
#define DMA_BUF_REMAP			0
#endif

#if defined(MTK_ETH_USE_I2P5G_PHY)
/* Using internal 2.5G PHY */
#define PHY_MODE			1
#define GMAC_ID				1
#define GMAC_FORCE_MODE			0
#define GMAC_INTERFACE_MODE		PHY_INTERFACE_MODE_XGMII
#define MTK_ETH_PHY_ADDR		15
#else
/* Using built-in switch */
#define PHY_MODE			0
#define GMAC_ID				0
#define GMAC_FORCE_MODE			1
#define GMAC_FORCE_SPEED		10000
#define GMAC_FORCE_FULL_DUPLEX		1
#define GMAC_INTERFACE_MODE		PHY_INTERFACE_MODE_USXGMII
#define SWITCH_MT7988
#endif

#endif /* _PLAT_ETH_DEF_H_ */
