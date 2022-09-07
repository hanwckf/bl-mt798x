/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7629_DEF_H
#define MT7629_DEF_H

#if RESET_TO_BL31
#error "MT7629 is incompatible with RESET_TO_BL31!"
#endif

#define XTAL_CLK			40000000
#define ARCH_TIMER_CLK			20000000

#define MT7629_PRIMARY_CPU		0x0

/* Register base address */

#define INFRACFG_AO_BASE		0x10000000
#define PERICFG_BASE			0x10002000
#define SRAMROM_SEC_BASE		0x10000800
#define SRAMROM_SEC_CTRL		(SRAMROM_SEC_BASE + 0x4)
#define SRAMROM_SEC_ADDR		(SRAMROM_SEC_BASE + 0x8)

#define SPM_BASE			0x10006000
#define MCUCFG_BASE			0x10200000
#define EMI_BASE			0x10203000
#define EFUSE_BASE			0x10206000
#define APMIXED_BASE			0x10209000
#define TRNG_BASE			0x1020f000
#define TOPCKGEN_BASE			0x10210000
#define RGU_BASE			0x10212000
#define DDRPHY_BASE			0x10213000
#define DRAMC_AO_BASE			0x10214000
#define DRAMC_NAO_BASE			0x1020e000
#define GPIO_BASE			0x10210000
#define NFI_BASE			0x1100d000
#define NFIECC_BASE			0x1100e000

/* Watchdog */
#define MTK_WDT_BASE			RGU_BASE
#define MTK_WDT_SWRST			(MTK_WDT_BASE + 0x14)
#define MTK_WDT_MODE_EXTEN		0x0004
#define MTK_WDT_MODE_IRQ		0x0008
#define MTK_WDT_MODE_DUAL_MODE		0x0040
#define MTK_WDT_SWRST_KEY		0x1209
#define MTK_WDT_MODE_KEY		0x22000000
#define MTK_WDT_MODE_ENABLE		0x0001

/* GIC-400 & Interrupt handling */
#define BASE_GIC_BASE			0x10310000
#define BASE_GICD_BASE			0x10310000
#define BASE_GICC_BASE			0x10320000
#define BASE_GICR_BASE			0	/* no GICR in GIC-400 */
#define BASE_GICH_BASE			0x10340000
#define BASE_GICV_BASE			0x10360000
#define GIC_PRIVATE_SIGNALS     	32

#define INT_POL_CTL0			0x10200620
#define INT_POL_SECCTL0			0x10200a00
#define INT_POL_SECCTL_NUM		8

/* CCI-400 */
#define CCI_BASE			0x10390000
#define CCI_CLUSTER0_SL_IFACE_IX	4
#define CCI_CLUSTER1_SL_IFACE_IX	3
#define CCI_SEC_ACCESS			0x8

/* FIXME: Aggregate of all devices in the first GB */
#define MTK_DEV_RNG0_BASE		0x10000000
#define MTK_DEV_RNG0_SIZE		0x400000
#define MTK_DEV_RNG1_BASE		0x11000000
#define MTK_DEV_RNG1_SIZE		0x1000000

/* UART */
#define UART0_BASE			0x11002000
#define UART_BAUDRATE			115200

/* System counter frequency */
#define SYS_COUNTER_FREQ_IN_TICKS	12500000
#define SYS_COUNTER_FREQ_IN_TICKS2	20000000

/* FIQ platform related define */
#define MT_IRQ_SEC_SGI_0		8
#define MT_IRQ_SEC_SGI_1		9
#define MT_IRQ_SEC_SGI_2		10
#define MT_IRQ_SEC_SGI_3		11
#define MT_IRQ_SEC_SGI_4		12
#define MT_IRQ_SEC_SGI_5		13
#define MT_IRQ_SEC_SGI_6		14
#define MT_IRQ_SEC_SGI_7		15

#endif /* MT7629_DEF_H */
