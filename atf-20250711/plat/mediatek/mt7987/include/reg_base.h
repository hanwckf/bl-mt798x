/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef _MT7987_REG_BASE_H_
#define _MT7987_REG_BASE_H_

/* GIC */
#define GIC_BASE				0x0C000000
/* INTER_SRAM */
#define INTER_SRAM				0x00100000
/* SRAMROM */
#define ROM_BASE				0x00000000
/* SRAMROM */
#define RAM_BASE				0x00100000
/* L2C */
#define L2C_BASE				0x00200000
/* HW_VER CID */
#define HW_VER_CID_BASE				0x08000000
/* INFRA_CFG_AO */
#define INFRACFG_AO_BASE			0x10001000
/* INFRA_CFG_AO_MEM */
#define INFRACFG_AO_MEM_BASE			0x10002000
/* APXGPT */
#define APXGPT_BASE				0x10008000
/* SYS_TIMER */
#define SYS_TIMER_BASE				0x10017000
/* CKSYS */
#define TOPCKGEN_BASE				0x1001B000
/* WDT */
#define TOPRGU_BASE				0x1001C000
/* DRM */
#define DRM_BASE				0x1001D000
/* CKSYS */
#define APMIXEDSYS_BASE				0x1001E000
/* GPIO */
#define GPIO_BASE				0x1001F000
/* TOP Misc */
#define TOP_MISC_BASE				0x10021000
/* PWM */
#define PWM_BASE				0x10048000
/* SGMII0 */
#define SGMII0_BASE				0x10060000
/* SGMII1 */
#define SGMII1_BASE				0x10070000
/* TRNG */
#define TRNG_BASE				0x1020F000
/* CQ_DMA */
#define CQ_DMA_BASE				0x10212000
/* EMI_APB */
#define EMI_APB_BASE				0x10219000
/* EMI_FAKE_ENGINE0 */
#define EMI_FAKE_ENGINE0_BASE			0x10225000
/* EMI_MPU_APB */
#define EMI_MPU_APB_BASE			0x10226000
/* EMI_FAKE_ENGINE1 */
#define EMI_FAKE_ENGINE1_BASE			0x10227000
/* MCUSYS_CFGREG_APB */
#define MCUSYS_CFGREG_APB_BASE			0x10390000
/* MCUSYS_CFGREG_APB-1 */
#ifdef MT7987_FPGA_CA73
#define MCUSYS_CFGREG_APB_1_BASE		0x100E0000
#else
#define MCUSYS_CFGREG_APB_1_BASE		0x10400000
#endif
#define MCUCFG_BASE				MCUSYS_CFGREG_APB_1_BASE

/* UART0 */
#define UART0_BASE				0x11000000
/* UART1 */
#define UART1_BASE				0x11000100
/* UART2 */
#define UART2_BASE				0x11000200
/* NFI */
#define NFI_BASE				0x11001000
/* NFI_ECC */
#define NFI_ECC_BASE				0x11002000
/* I2C */
#define I2C_BASE				0x11003000
/*
 * - For mt7987, although base address of spi0 & spi1 are still 0x11007000 and
 * 0x11008000, all subsequent SPI CRs starting from spi0_cfg have an 0x800
 * additional address shift which differ from mt7981/mt7986/mt7988. Defining spi
 * CRs based on different platforms in common part (mtk_spi.c) would make code
 * less readable. Hence, we define base address of spi0 and spi1 to 0x11007800
 * and 0x11008800 for mt7987.
 *
 * - 0x11007000 ~ 0x11007014 are spi CRs related to security and are not being
 * used in griffin now.
 */
/* SPI0 */
#define SPI0_BASE				0x11007800
/* SPI1 */
#define SPI1_BASE				0x11008800
/* MDSC0 */
#define MSDC0_BASE				0x11230000
/* EAST0 */
#define PCIE_PHYD_L0_BASE			0x11C00000
/* EAST1 */
#define PCIE_PHYD_L1_BASE			0x11C10000
/* EFUSE */
#define EFUSE_BASE				0x11D30000
/* MDSC0 TOP */
#define MSDC0_TOP_BASE				0x11F50000
/* NETSYS */
#define NETSYS_BASE				0x15000000
/* DEVAPC_FMEM_AO */
#define DEVAPC_FMEM_AO_BASE			0x1A0F0000
/* DEVAPC_INFRA_AO */
#define DEVAPC_INFRA_AO_BASE			0x1A100000
/* DEVAPC_INFRA_PDN */
#define DEVAPC_INFRA_PDN_BASE			0x1A110000

#endif /* _MT7987_REG_BASE_H_ */
