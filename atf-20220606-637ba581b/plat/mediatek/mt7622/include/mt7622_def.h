/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7622_DEF_H
#define MT7622_DEF_H

#if RESET_TO_BL31
#error "MT7622 is incompatible with RESET_TO_BL31!"
#endif

#define MT7622_PRIMARY_CPU		0x0

/* Special value used to verify platform parameters from BL2 to BL3-1 */
#define MT_BL31_PLAT_PARAM_VAL		0x0f1e2d3c4b5a6978ULL

/* Register base address */

#define INFRACFG_AO_BASE		0x10000000
#define PERICFG_BASE			0x10002000
#define SRAMROM_SEC_BASE		0x10000800
#define SRAMROM_SEC_CTRL		(SRAMROM_SEC_BASE + 0x4)
#define SRAMROM_SEC_ADDR		(SRAMROM_SEC_BASE + 0x8)

#define PMIC_WRAP_BASE			0x10001000
#define SPM_BASE			0x10006000
#define MCUCFG_BASE			0x10200000
#define EFUSE_BASE			0x10206000
#define APMIXED_BASE			0x10209000
#define TRNG_BASE			0x1020f000
#define TOPCKGEN_BASE			0x10210000
#define GPIO_BASE			0x10211000
#define RGU_BASE			0x10212000
#define DDRPHY_BASE			0x10213000
#define DRAMC0_BASE			0x10214000
#define NFI_BASE			0x1100d000
#define NFIECC_BASE			0x1100e000
#define AUDIO_BASE			0x11220000
#define MSDC0_BASE			0x11230000
#define MSDC1_BASE			0x11240000

#define RGU_STRAP_PAR			(RGU_BASE + 0x60)
#define RGU_INPUT_TYPE			BIT(1)

/* Watchdog */
#define MTK_WDT_BASE			RGU_BASE

#define MTK_WDT_MODE			(MTK_WDT_BASE + 0x00)
#define MTK_WDT_MODE_ENABLE		0x0001
#define MTK_WDT_MODE_EXTEN		0x0004
#define MTK_WDT_MODE_IRQ		0x0008
#define MTK_WDT_MODE_DUAL_MODE		0x0040
#define MTK_WDT_MODE_DDR_RESERVE	0x0080
#define MTK_WDT_MODE_KEY		0x22000000

#define MTK_WDT_SWRST			(MTK_WDT_BASE + 0x14)
#define MTK_WDT_SWRST_KEY		0x1209

#define MTK_WDT_DEBUG_CTL		(MTK_WDT_BASE + 0x40)
#define MTK_RG_DRAMC_SREF		0x00100
#define MTK_RG_DRAMC_ISO		0x00200
#define MTK_RG_CONF_ISO			0x00400
#define MTK_DDR_RESERVE_STA		0x10000
#define MTK_DDR_SREF_STA		0x20000
#define MTK_DEBUG_CTL_KEY		0x59000000

/* Top-Clock Generator */
#define CLK_CFG_0			(TOPCKGEN_BASE + 0x40)
#define CLK_PDN_AXI			0x80
#define CLK_AXI_SEL_M			0x07
#define CLK_AXI_SEL_S			0

#define CLK_CFG_1			(TOPCKGEN_BASE + 0x50)

#define CLK_CFG_2			(TOPCKGEN_BASE + 0x60)
#define CLK_PDN_MSDC50_0		0x80000000
#define CLK_MSDC50_0_SEL_M		0x7000000
#define CLK_MSDC50_0_SEL_S		24

#define CLK_CFG_3			(TOPCKGEN_BASE + 0x70)
#define CLK_PDN_MSDC30_1		0x8000
#define CLK_MSDC30_1_SEL_M		0x700
#define CLK_MSDC30_1_SEL_S		8
#define CLK_PDN_MSDC30_0		0x80
#define CLK_MSDC30_0_SEL_M		0x07
#define CLK_MSDC30_0_SEL_S		0

#define CLK_CFG_4			(TOPCKGEN_BASE + 0x80)
#define CLK_CFG_5			(TOPCKGEN_BASE + 0x90)
#define CLK_CFG_6			(TOPCKGEN_BASE + 0xa0)
#define CLK_CFG_7			(TOPCKGEN_BASE + 0xb0)
#define CLK_AUDDIV_0			(TOPCKGEN_BASE + 0x120)
#define CLK_AUDDIV_1			(TOPCKGEN_BASE + 0x124)
#define CLK_AUDDIV_2			(TOPCKGEN_BASE + 0x128)
#define CLK_SCP_CFG_0			(TOPCKGEN_BASE + 0x200)
#define CLK_SCP_CFG_1			(TOPCKGEN_BASE + 0x204)

/* GPIO */
#define GPIO_MODE0			(GPIO_BASE + 0x300)
#define NAND_MODE_M			0xf00000
#define NAND_MODE_S			20

#define GPIO_MODE1			(GPIO_BASE + 0x310)
#define GPIO21_MODE_M			0xf0000000
#define GPIO21_MODE_S			28
#define GPIO20_MODE_M			0xf000000
#define GPIO20_MODE_S			24
#define GPIO19_MODE_M			0xf00000
#define GPIO19_MODE_S			20
#define GPIO18_MODE_M			0xf0000
#define GPIO18_MODE_S			16

#define GPIO_MODE2			(GPIO_BASE + 0x320)
#define GPIO17_MODE_M			0xf000000
#define GPIO17_MODE_S			24
#define GPIO16_MODE_M			0xf00000
#define GPIO16_MODE_S			20

#define GPIO_G1_SMT			(GPIO_BASE + 0x1920)
#define GPIO_G1_PULLUP			(GPIO_BASE + 0x1930)
#define GPIO_G1_PULLDN			(GPIO_BASE + 0x1940)
#define GPIO_G1_E4			(GPIO_BASE + 0x1960)
#define GPIO_G1_E8			(GPIO_BASE + 0x1970)
#define GPIO_G2_SMT			(GPIO_BASE + 0x1a20)
#define GPIO_G2_PULLUP			(GPIO_BASE + 0x1a30)
#define GPIO_G2_PULLDN			(GPIO_BASE + 0x1a40)
#define GPIO_G2_E4			(GPIO_BASE + 0x1a60)
#define GPIO_G2_E8			(GPIO_BASE + 0x1a70)

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
#define UART_CLOCK			25000000

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

#endif /* MT7622_DEF_H */
