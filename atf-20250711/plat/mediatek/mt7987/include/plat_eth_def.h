/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 */

#ifndef _PLAT_ETH_DEF_H_
#define _PLAT_ETH_DEF_H_

#include <stdbool.h>
#include <common/bl_common.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <reg_base.h>

/* SoC-specific definitions */
#define SGMII_BASE			SGMII0_BASE
#define ETHSYS_BASE			NETSYS_BASE
#define FE_BASE				(ETHSYS_BASE + 0x100000)

#define PDMA_BASE			PDMA_V3_BASE
#define ANA_RG3_OFFS			0x128
#define GDMA_COUNT			3

#define SWITCH_RESET_GPIO

#define TXD_SIZE			32
#define RXD_SIZE			32

#define SOC_CAPS			(MTK_NETSYS_V3 | MTK_GMAC2_U3_QPHY | MTK_INFRA | \
					 MTK_RSTCTRL_PPE1 | MTK_RSTCTRL_PPE2)

#if defined(IMAGE_BL31)
#define DMA_BUF_ADDR			mtk_plat_eth_dma_buf_addr()
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
#define GMAC_FORCE_SPEED		2500
#define GMAC_FORCE_FULL_DUPLEX		1
#define GMAC_INTERFACE_MODE		PHY_INTERFACE_MODE_2500BASEX
#define SWITCH_MT7531
#define SWITCH_AN8855
#endif

static inline void mtk_plat_switch_reset(bool assert)
{
	mmio_write_32(GPIO_BASE + 0x358, 0x00000700);	/* GPIO_MODE5_CLR */
	mmio_write_32(GPIO_BASE + 0x014, BIT(10));	/* GPIO_DIR1_SET */

	if (assert)
		mmio_write_32(GPIO_BASE + 0x118, BIT(10));	/* GPIO_DOUT0_CLR */
	else
		mmio_write_32(GPIO_BASE + 0x114, BIT(10));	/* GPIO_DOUT0_SET */
}

static inline uintptr_t mtk_plat_eth_dma_buf_addr(void)
{
	/* INTER_SRAM is unusable for MT7987 */
	return BL31_START - BL31_LOAD_OFFSET + TZRAM_SIZE;
}

#endif /* _PLAT_ETH_DEF_H_ */
