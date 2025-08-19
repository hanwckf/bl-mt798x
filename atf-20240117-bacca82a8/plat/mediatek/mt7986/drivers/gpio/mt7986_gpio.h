/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7986_GPIO_H
#define MT7986_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include <mt7986_def.h>
#include <plat/common/common_def.h>

/* GPIO mode */
#define GPIO_MODE2		(GPIO_BASE + 0x320)
#define GPIO_PIN22_S		24
#define GPIO_PIN23_S		28

#define GPIO_MODE3		(GPIO_BASE + 0x330)
#define   GPIO_PIN24_S		0
#define   GPIO_PIN25_S		4
#define   GPIO_PIN26_S		8
#define   GPIO_PIN27_S		12
#define   GPIO_PIN28_S		16
#define   GPIO_PIN29_S		20
#define   GPIO_PIN30_S		24
#define   GPIO_PIN31_S		28

#define GPIO_MODE4		(GPIO_BASE + 0x340)
#define   GPIO_PIN32_S		0
#define   GPIO_PIN33_S		4
#define   GPIO_PIN34_S		8
#define   GPIO_PIN35_S		12
#define   GPIO_PIN36_S		16
#define   GPIO_PIN37_S		20
#define   GPIO_PIN38_S		24

#define GPIO_MODE6		(GPIO_BASE + 0x360)
#define   GPIO_PIN50_S		8
#define   GPIO_PIN51_S		12
#define   GPIO_PIN52_S		16
#define   GPIO_PIN53_S		20
#define   GPIO_PIN54_S		24
#define   GPIO_PIN55_S		28


#define GPIO_MODE7		(GPIO_BASE + 0x370)
#define   GPIO_PIN56_S		0
#define   GPIO_PIN57_S		4
#define   GPIO_PIN58_S		8
#define   GPIO_PIN59_S		12
#define   GPIO_PIN60_S		16
#define   GPIO_PIN61_S		20

/* GPIO driving*/
#define GPIO_RT_DRV_CFG1	(IOCFG_RT_BASE + 0x10)
#define   SPI0_WP_DRV_S		27
#define   SPI0_MOSI_DRV_S	24
#define   SPI0_MISO_DRV_S	21
#define   SPI0_HOLD_DRV_S	18
#define   SPI0_CS_DRV_S		15
#define   SPI0_CLK_DRV_S	12

#define GPIO_LT_DRV_CFG0	(IOCFG_LT_BASE + 0x0)
#define   SPI2_WP_DRV_S		27
#define   SPI2_MOSI_DRV_S	24
#define   SPI2_MISO_DRV_S	21
#define   SPI2_HOLD_DRV_S	18
#define   SPI2_CS_DRV_S		15
#define   SPI2_CLK_DRV_S	12

/* GPIO PU/PD*/
#define GPIO_RT_PUPD_CFG0	(IOCFG_RT_BASE + 0x40)
#define   SPI0_PUPD_S		14
#define GPIO_RT_R0_CFG0		(IOCFG_RT_BASE + 0x50)
#define   SPI0_R0_S		14
#define GPIO_RT_R1_CFG0		(IOCFG_RT_BASE + 0x60)
#define   SPI0_R1_S		14

#define GPIO_LT_PUPD_CFG0	(IOCFG_LT_BASE + 0x30)
#define   SPI2_PUPD_S		4
#define GPIO_LT_R0_CFG0		(IOCFG_LT_BASE + 0x40)
#define   SPI2_R0_S		4
#define GPIO_LT_R1_CFG0		(IOCFG_LT_BASE + 0x50)
#define   SPI2_R1_S		4

#define MSDC_GPIO_DRV_CFG0	(IOCFG_RT_BASE + 0x0)
#define   EMMC51_CLK_DRV_S	0
#define   EMMC51_CMD_DRV_S	3
#define   EMMC51_DAT0_DRV_S	6
#define   EMMC51_DAT1_DRV_S	9
#define   EMMC51_DAT2_DRV_S	12
#define   EMMC51_DAT3_DRV_S	15
#define   EMMC51_DAT4_DRV_S	18
#define   EMMC51_DAT5_DRV_S	21
#define   EMMC51_DAT6_DRV_S	24
#define   EMMC51_DAT7_DRV_S	27

#define MSDC_GPIO_DRV_CFG1	(IOCFG_RT_BASE + 0x10)
#define   EMMC51_DSL_DRV_S	0
#define   EMMC51_RSTB_DRV_S	3

#define EMMC45_RSTB_DRV_S	9
#define EMMC45_DAT0_DRV_S	12
#define EMMC45_DAT3_DRV_S	15
#define EMMC45_DAT4_DRV_S	18
#define EMMC45_DAT2_DRV_S	21
#define EMMC45_DAT1_DRV_S	24
#define EMMC45_DAT5_DRV_S	27

#define MSDC_GPIO_DRV_CFG2	(IOCFG_RT_BASE + 0x20)
#define EMMC45_DAT6_DRV_S	0
#define EMMC45_CLK_DRV_S	3
#define EMMC45_CMD_DRV_S	6
#define EMMC45_DAT7_DRV_S	9

#define MSDC_GPIO_IES_CFG0	(IOCFG_RT_BASE + 0x30)
#define EMMC51_GPIO_IES_S	0
#define EMMC45_GPIO_IES_S	13

#define MSDC_GPIO_PUPD_CFG0	(IOCFG_RT_BASE + 0x40)
#define EMMC51_GPIO_PUPD_S	0
#define EMMC45_GPIO_PUPD_S	13

#define MSDC_GPIO_R0_CFG0	(IOCFG_RT_BASE + 0x50)
#define EMMC51_GPIO_R0_S	0
#define EMMC45_GPIO_R0_S	13

#define MSDC_GPIO_R1_CFG0	(IOCFG_RT_BASE + 0x60)
#define EMMC51_GPIO_R1_S	0
#define EMMC45_GPIO_R1_S	13

#define MSDC_GPIO_SMT_CFG0	(IOCFG_RT_BASE + 0xC0)
#define EMMC51_GPIO_SMT_S	0
#define EMMC45_GPIO_SMT_S	13

#define MAX_GPIO_PIN		101

/* GPIO PULL ENABLE*/
typedef enum {
	MT_GPIO_PULL_EN_UNSUPPORTED = -1,
	MT_GPIO_PULL_DISABLE   = 0,
	MT_GPIO_PULL_ENABLE    = 1,
	MT_GPIO_PULL_ENABLE_R0 = 2,
	MT_GPIO_PULL_ENABLE_R1 = 3,
	MT_GPIO_PULL_ENABLE_R0R1 = 4,

	MT_GPIO_PULL_EN_MAX,
	MT_GPIO_PULL_EN_DEFAULT = MT_GPIO_PULL_ENABLE,
} GPIO_PULL_EN;

void mtk_pin_init(void);
void mt7986_set_default_pinmux(void);

#endif /* MT_GPIO_H */
