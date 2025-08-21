/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef MT7987_GPIO_H
#define MT7987_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include <plat/common/common_def.h>

/*  Error Code No. */
#define RSUCCESS     0
#define ERACCESS     1
#define ERINVAL	     2
#define ERWRAPPER    3
#define MAX_GPIO_PIN MT_GPIO_BASE_MAX

/* GPIO base address*/
#define IOCFG_RB_BASE  (0x11D00000)
#define IOCFG_LB_BASE  (0x11E00000)
#define IOCFG_RT1_BASE (0x11F00000)
#define IOCFG_RT2_BASE (0x11F40000)
#define IOCFG_TL_BASE  (0x11F60000)

/* GPIO mode */
/* The following defines may not be used in atf, just keep them now.
#define GPIO_MODE2   (GPIO_BASE + 0x320)
#define GPIO_PIN16_S (0)
#define GPIO_PIN17_S (4)
#define GPIO_PIN18_S (8)
#define GPIO_PIN19_S (12)
#define GPIO_PIN20_S (16)
#define GPIO_PIN21_S (20)

#define GPIO_MODE3   (GPIO_BASE + 0x330)
#define GPIO_PIN24_S (0)
#define GPIO_PIN25_S (4)
#define GPIO_PIN26_S (8)
#define GPIO_PIN27_S (12)
#define GPIO_PIN28_S (16)

#define GPIO_MODE4   (GPIO_BASE + 0x340)
#define GPIO_PIN33_S (4)
#define GPIO_PIN34_S (8)
#define GPIO_PIN35_S (12)
#define GPIO_PIN36_S (16)
#define GPIO_PIN37_S (20)
#define GPIO_PIN38_S (24)
*/

/* GPIO driving*/
/* SPI0 */
#define GPIO_RT1_DRV_CFG0 (IOCFG_RT1_BASE)
#define SPI0_CLK_DRV_S	 (9)
#define SPI0_CS_DRV_S	 (12)
#define SPI0_HOLD_DRV_S	 (15)
#define SPI0_MISO_DRV_S	 (18)
#define SPI0_MOSI_DRV_S	 (21)
#define SPI0_WP_DRV_S	 (24)

/* SPI2 */
#define GPIO_RT1_DRV_CFG1 (IOCFG_RT1_BASE + 0x10)
#define SPI2_CLK_DRV_S	 (9)
#define SPI2_MOSI_DRV_S	 (12)

#define GPIO_RT2_DRV_CFG0 (IOCFG_RT2_BASE)
#define SPI2_CS_DRV_S	 (21)
#define SPI2_HOLD_DRV_S	 (24)
#define SPI2_MISO_DRV_S	 (27)

#define GPIO_RT2_DRV_CFG1 (IOCFG_RT2_BASE + 0x10)
#define SPI2_WP_DRV_S	 (0)

/* GPIO PU/PD*/
#define GPIO_RT1_PUPD_CFG0	(IOCFG_RT1_BASE + 0x30)
#define   SPI0_PUPD_S		(3)  			// spi0: bit3 ~ bit8
#define   SPI2_PUPD_S1		(13) 			// spi2_clk, spi2_mosi (bit13 ~ bit14)
#define GPIO_RT2_PUPD_CFG0	(IOCFG_RT2_BASE + 0x30)
#define   SPI2_PUPD_S2		(7)			// spi2_cs, spi2_hold, spi2_miso, spi2_wp (bit7 ~ bit10)

#define GPIO_RT1_R0_CFG0	(IOCFG_RT1_BASE + 0x40)
#define   SPI0_R0_S		(3)			// spi0: bit3 ~ bit8
#define   SPI2_R0_S1		(13)			// spi2_clk, spi2_mosi (bit13 ~ bit14)
#define GPIO_RT2_R0_CFG0	(IOCFG_RT2_BASE + 0x40)	
#define   SPI2_R0_S2		(7)			// spi2_cs, spi2_hold, spi2_miso, spi2_wp (bit7 ~ bit10)

#define GPIO_RT1_R1_CFG0	(IOCFG_RT1_BASE + 0x50)
#define   SPI0_R1_S		(3)			// spi0: bit3 ~ bit8
#define   SPI2_R1_S1		(13)			// spi2_clk, spi2_mosi (bit13 ~ bit14)
#define GPIO_RT2_R1_CFG0	(IOCFG_RT2_BASE + 0x50)
#define   SPI2_R1_S2		(7)			// spi2_cs, spi2_hold, spi2_miso, spi2_wp (bit7 ~ bit10)

/* For eMMC and SDCard */
#define MSDC_GPIO_DRV_CFG0   (IOCFG_RT1_BASE + 0x00)
#define MSDC_GPIO_DRV_CFG1   (IOCFG_RT1_BASE + 0x10)
#define MSDC_GPIO_IES_CFG0   (IOCFG_RT1_BASE + 0x20)
#define MSDC_GPIO_R0_CFG0    (IOCFG_RT1_BASE + 0x40)
#define MSDC_GPIO_R1_CFG0    (IOCFG_RT1_BASE + 0x50)
#define MSDC_GPIO_RDSEL_CFG0 (IOCFG_RT1_BASE + 0x60)
#define MSDC_GPIO_RDSEL_CFG1 (IOCFG_RT1_BASE + 0x70)
#define MSDC_GPIO_RDSEL_CFG2 (IOCFG_RT1_BASE + 0x80)
#define MSDC_GPIO_RDSEL_CFG3 (IOCFG_RT1_BASE + 0x90)
#define MSDC_GPIO_SMT_CFG0   (IOCFG_RT1_BASE + 0xA0)
#define MSDC_GPIO_TDSEL_CFG0 (IOCFG_RT1_BASE + 0xC0)
#define MSDC_GPIO_TDSEL_CFG1 (IOCFG_RT1_BASE + 0xD0)

/* Enumeration for GPIO pin */
typedef enum GPIO_PIN {
	GPIO_UNSUPPORTED = -1,

	GPIO0,
	GPIO1,
	GPIO2,
	GPIO3,
	GPIO4,
	GPIO5,
	GPIO6,
	GPIO7,
	GPIO8,
	GPIO9,
	GPIO10,
	GPIO11,
	GPIO12,
	GPIO13,
	GPIO14,
	GPIO15,
	GPIO16,
	GPIO17,
	GPIO18,
	GPIO19,
	GPIO20,
	GPIO21,
	GPIO22,
	GPIO23,
	GPIO24,
	GPIO25,
	GPIO26,
	GPIO27,
	GPIO28,
	GPIO29,
	GPIO30,
	GPIO31,
	GPIO32,
	GPIO33,
	GPIO34,
	GPIO35,
	GPIO36,
	GPIO37,
	GPIO38,
	GPIO39,
	GPIO40,
	GPIO41,
	GPIO42,
	GPIO43,
	GPIO44,
	GPIO45,
	GPIO46,
	GPIO47,
	GPIO48,
	GPIO49,
	MT_GPIO_BASE_MAX
} GPIO_PIN;

/* GPIO MODE CONTROL VALUE*/
typedef enum {
	GPIO_MODE_UNSUPPORTED = -1,
	GPIO_MODE_GPIO = 0,
	GPIO_MODE_00 = 0,
	GPIO_MODE_01,
	GPIO_MODE_02,
	GPIO_MODE_03,
	GPIO_MODE_04,
	GPIO_MODE_05,
	GPIO_MODE_06,

	GPIO_MODE_MAX,
	GPIO_MODE_DEFAULT = GPIO_MODE_00,
} GPIO_MODE;

/* GPIO DIRECTION */
typedef enum {
	MT_GPIO_DIR_UNSUPPORTED = -1,
	MT_GPIO_DIR_OUT = 0,
	MT_GPIO_DIR_IN = 1,
	MT_GPIO_DIR_MAX,
	MT_GPIO_DIR_DEFAULT = MT_GPIO_DIR_IN,
} GPIO_DIR;

/* GPIO PULL ENABLE*/
typedef enum {
	MT_GPIO_PULL_EN_UNSUPPORTED = -1,
	MT_GPIO_PULL_DISABLE = 0,
	MT_GPIO_PULL_ENABLE = 1,
	MT_GPIO_PULL_ENABLE_R0 = 2,
	MT_GPIO_PULL_ENABLE_R1 = 3,
	MT_GPIO_PULL_ENABLE_R0R1 = 4,

	MT_GPIO_PULL_EN_MAX,
	MT_GPIO_PULL_EN_DEFAULT = MT_GPIO_PULL_ENABLE,
} GPIO_PULL_EN;

/* GPIO PULL-UP/PULL-DOWN*/
typedef enum {
	MT_GPIO_PULL_UNSUPPORTED = -1,
	MT_GPIO_PULL_NONE = 0,
	MT_GPIO_PULL_UP = 1,
	MT_GPIO_PULL_DOWN = 2,
	MT_GPIO_PULL_MAX,
	MT_GPIO_PULL_DEFAULT = MT_GPIO_PULL_DOWN
} GPIO_PULL;

/* GPIO OUTPUT */
typedef enum {
	MT_GPIO_OUT_UNSUPPORTED = -1,
	MT_GPIO_OUT_ZERO = 0,
	MT_GPIO_OUT_ONE = 1,

	MT_GPIO_OUT_MAX,
	MT_GPIO_OUT_DEFAULT = MT_GPIO_OUT_ZERO,
	MT_GPIO_DATA_OUT_DEFAULT = MT_GPIO_OUT_ZERO, /*compatible with DCT*/
} GPIO_OUT;

/* GPIO INPUT */
typedef enum {
	MT_GPIO_IN_UNSUPPORTED = -1,
	MT_GPIO_IN_ZERO = 0,
	MT_GPIO_IN_ONE = 1,

	MT_GPIO_IN_MAX,
} GPIO_IN;

typedef struct {
	uint32_t val;
	uint32_t set;
	uint32_t rst;
	uint32_t _align1;
} VAL_REGS;

typedef struct {
	VAL_REGS dir[4];
	uint8_t rsv00[192];
	VAL_REGS dout[4];
	uint8_t rsv01[192];
	VAL_REGS din[4];
	uint8_t rsv02[192];
	VAL_REGS mode[11];
} GPIO_REGS;

typedef enum {
	MT_PUPD_UNSUPPORTED = -1,
	MT_PU_PD_TYPE = 0,
	MT_PUPD_R0_R1_TYPE = 1,
} MTK_PIN_PU_PD_TYPE;

typedef enum {
	MT_IOCFG_UNSPUPPORTED = -1,
	MT_IOCFG_RB = 0x11,
	MT_IOCFG_LB = 0x12,
	MT_IOCFG_RT1 = 0x13,
	MT_IOCFG_RT2 = 0x14,
	MT_IOCFG_TL = 0x15,
} MTK_PIN_FIELD;

#define PIN(_id, _flag, _base, _offset, _bit) {		\
		.id = _id,				\
		.flag = _flag,				\
		.base = _base,				\
		.offset = _offset,			\
		.bit = _bit,				\
	}

/*
	* mt_pin_info
		(1) id : pin number
		(2) flag : pupd or pu/pd
			- 1 for pupd/r0/r1 type,
			- 0 for pu/pd
		(3) base : decide which pin field base
		(4) offset : CR offset of pupd or pu/pd
			- pupd/r0/r1 type is pupd offset
			- pu/pd type is pu offset.
		(5) bit : correspodded pin bit of pupd or pu/pd CR
 */
struct mt_pin_info {
	uint8_t id;
	uint8_t flag;
	uint16_t base;
	uint16_t offset;
	uint8_t bit;
};

static const struct mt_pin_info mt7987_pin_infos[] = {
	PIN(0, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 3),
	PIN(1, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 2),
	PIN(2, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 11),
	PIN(3, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 2),
	PIN(4, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 1),
	PIN(5, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 3),
	PIN(6, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 0),
	PIN(7, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 4),
	PIN(8, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RB, 0x20, 2),
	PIN(9, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RB, 0x20, 1),
	PIN(10, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RB, 0x20, 0),
	PIN(11, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RB, 0x20, 3),
	PIN(12, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RB, 0x20, 4),
	PIN(13, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 0),
	PIN(14, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 15),
	PIN(15, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 3),
	PIN(16, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 7),
	PIN(17, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 6),
	PIN(18, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 4),
	PIN(19, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 5),
	PIN(20, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 8),
	PIN(21, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 9),
	PIN(22, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 12),
	PIN(23, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 11),
	PIN(24, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 10),
	PIN(25, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 13),
	PIN(26, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 14),
	PIN(27, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 9),
	PIN(28, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 7),
	PIN(29, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 8),
	PIN(30, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 10),
	PIN(31, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 5),
	PIN(32, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 6),
	PIN(33, MT_PU_PD_TYPE, MT_IOCFG_LB, 0x40, 2),
	PIN(34, MT_PU_PD_TYPE, MT_IOCFG_LB, 0x40, 0),
	PIN(35, MT_PU_PD_TYPE, MT_IOCFG_LB, 0x40, 4),
	PIN(36, MT_PU_PD_TYPE, MT_IOCFG_LB, 0x40, 3),
	PIN(37, MT_PU_PD_TYPE, MT_IOCFG_LB, 0x40, 1),
	PIN(38, MT_PU_PD_TYPE, MT_IOCFG_LB, 0x40, 5),
	PIN(39, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 1),
	PIN(40, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT1, 0x30, 2),
	PIN(41, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 0),
	PIN(42, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 1),
	PIN(43, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 4),
	PIN(44, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 5),
	PIN(45, MT_PUPD_R0_R1_TYPE, MT_IOCFG_RT2, 0x30, 6),
	PIN(46, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 9),
	PIN(47, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 10),
	PIN(48, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 7),
	PIN(49, MT_PUPD_R0_R1_TYPE, MT_IOCFG_TL, 0x30, 8),
};

void mtk_pin_init(void);
void mt7987_set_default_pinmux(void);
void mt_set_gpio_dir(int gpio, int direction);
int mt_get_gpio_dir(int gpio);
void mt_set_gpio_pull(int gpio, int pull);
int mt_get_gpio_pull(int gpio);
void mt_set_gpio_out(int gpio, int value);
int mt_get_gpio_in(int gpio);
void mt_set_pinmux_mode(int gpio, int mode);
int mt_get_pinmux_mode(int gpio);

#endif /* MT_GPIO_H */
