/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7988_GPIO_H
#define MT7988_GPIO_H

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
#define GPIO_BASE     (0x1001F000)
#define IOCFG_TR_BASE (0x11C10000)
#define IOCFG_BR_BASE (0x11D00000)
#define IOCFG_RB_BASE (0x11D20000)
#define IOCFG_LB_BASE (0x11E00000)
#define IOCFG_TL_BASE (0x11F00000)

/* GPIO mode */
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

/* GPIO driving*/
#define GPIO_RB_DRV_CFG1 (IOCFG_RB_BASE + 0x10)
#define SPI0_CS_DRV_S	 (27)
#define SPI0_CLK_DRV_S	 (24)

#define GPIO_RB_DRV_CFG2 (IOCFG_RB_BASE + 0x20)
#define SPI0_WP_DRV_S	 (9)
#define SPI0_MOSI_DRV_S	 (6)
#define SPI0_MISO_DRV_S	 (3)
#define SPI0_HOLD_DRV_S	 (0)

#define SPI2_CS_DRV_S	 (27)
#define SPI2_CLK_DRV_S	 (24)

#define GPIO_RB_DRV_CFG3 (IOCFG_RB_BASE + 0x30)
#define SPI2_WP_DRV_S	 (9)
#define SPI2_MOSI_DRV_S	 (6)
#define SPI2_MISO_DRV_S	 (3)
#define SPI2_HOLD_DRV_S	 (0)

/* GPIO PU/PD*/
#define GPIO_RB_PUPD_CFG0	(IOCFG_RB_BASE + 0x70)
#define   SPI0_PUPD_S		(18)
#define   SPI2_PUPD_S1		(28)
#define GPIO_RB_PUPD_CFG1	(IOCFG_RB_BASE + 0x80)
#define   SPI2_PUPD_S2		(0)

#define GPIO_RB_R0_CFG0		(IOCFG_RB_BASE + 0x90)
#define   SPI0_R0_S		(18)
#define   SPI2_R0_S1		(28)
#define GPIO_RB_R0_CFG1		(IOCFG_RB_BASE + 0xa0)
#define   SPI2_R0_S2		(0)

#define GPIO_RB_R1_CFG0		(IOCFG_RB_BASE + 0xb0)
#define   SPI0_R1_S		(18)
#define   SPI2_R1_S1		(28)
#define GPIO_RB_R1_CFG1		(IOCFG_RB_BASE + 0xc0)
#define   SPI2_R1_S2		(0)

/* For eMMC */
#define MSDC_GPIO_IES_CFG0   (IOCFG_RB_BASE + 0x50)
#define MSDC_GPIO_SMT_CFG0   (IOCFG_RB_BASE + 0x140)
#define MSDC_GPIO_R0_CFG0    (IOCFG_RB_BASE + 0x90)
#define MSDC_GPIO_R1_CFG0    (IOCFG_RB_BASE + 0xB0)
#define MSDC_GPIO_DRV_CFG0   (IOCFG_RB_BASE + 0x00)
#define MSDC_GPIO_DRV_CFG1   (IOCFG_RB_BASE + 0x10)
#define MSDC_GPIO_RDSEL_CFG0 (IOCFG_RB_BASE + 0xD0)
#define MSDC_GPIO_RDSEL_CFG1 (IOCFG_RB_BASE + 0xE0)
#define MSDC_GPIO_RDSEL_CFG2 (IOCFG_RB_BASE + 0xF0)
#define MSDC_GPIO_TDSEL_CFG0 (IOCFG_RB_BASE + 0x180)
#define MSDC_GPIO_TDSEL_CFG1 (IOCFG_RB_BASE + 0x190)

/* For SD-Card */
#define SD_GPIO_IES_CFG0   (IOCFG_RB_BASE + 0x50)
#define SD_GPIO_IES_CFG1   (IOCFG_RB_BASE + 0x60)
#define SD_GPIO_SMT_CFG0   (IOCFG_RB_BASE + 0x140)
#define SD_GPIO_SMT_CFG1   (IOCFG_RB_BASE + 0x150)
#define SD_GPIO_R0_CFG0	   (IOCFG_RB_BASE + 0x90)
#define SD_GPIO_R0_CFG1	   (IOCFG_RB_BASE + 0xA0)
#define SD_GPIO_R1_CFG0	   (IOCFG_RB_BASE + 0xB0)
#define SD_GPIO_R1_CFG1	   (IOCFG_RB_BASE + 0xC0)
#define SD_GPIO_DRV_CFG2   (IOCFG_RB_BASE + 0x20)
#define SD_GPIO_DRV_CFG3   (IOCFG_RB_BASE + 0x30)
#define SD_GPIO_RDSEL_CFG4 (IOCFG_RB_BASE + 0x110)
#define SD_GPIO_RDSEL_CFG5 (IOCFG_RB_BASE + 0x120)
#define SD_GPIO_RDSEL_CFG6 (IOCFG_RB_BASE + 0x130)
#define SD_GPIO_TDSEL_CFG3 (IOCFG_RB_BASE + 0x1B0)
#define SD_GPIO_TDSEL_CFG4 (IOCFG_RB_BASE + 0x1C0)

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
	GPIO50,
	GPIO51,
	GPIO52,
	GPIO53,
	GPIO54,
	GPIO55,
	GPIO56,
	GPIO57,
	GPIO58,
	GPIO59,
	GPIO60,
	GPIO61,
	GPIO62,
	GPIO63,
	GPIO64,
	GPIO65,
	GPIO66,
	GPIO67,
	GPIO68,
	GPIO69,
	GPIO70,
	GPIO71,
	GPIO72,
	GPIO73,
	GPIO74,
	GPIO75,
	GPIO76,
	GPIO77,
	GPIO78,
	GPIO79,
	GPIO80,
	GPIO81,
	GPIO82,
	GPIO83,
	GPIO84,
	GPIO85,
	GPIO86,
	GPIO87,
	GPIO88,
	GPIO89,
	GPIO90,
	GPIO91,
	GPIO92,
	GPIO93,
	GPIO94,
	GPIO95,
	GPIO96,
	GPIO97,
	GPIO98,
	GPIO99,
	GPIO100,
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
	GPIO_MODE_07,

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

#define PIN(_id, _flag, _base, _offset, _bit) {		\
		.id = _id,				\
		.flag = _flag,				\
		.base = _base,				\
		.offset = _offset,			\
		.bit = _bit,				\
	}

struct mt_pin_info {
	uint8_t id;
	uint8_t flag;
	uint16_t base;
	/* pupd/r0/r1 type is pupd offset,
	 * pu/pd type is pu offsert.
	 */
	uint16_t offset;
	uint8_t bit;
};

static const struct mt_pin_info mt7988_pin_infos[] = {
	PIN(0, 1, 0x15, 0x50, 7),   PIN(1, 1, 0x15, 0x50, 8),
	PIN(2, 1, 0x15, 0x50, 5),   PIN(3, 1, 0x15, 0x50, 6),
	PIN(4, 1, 0x15, 0x50, 0),   PIN(5, 1, 0x15, 0x50, 3),
	PIN(6, 1, 0x15, 0x50, 4),
	PIN(7, 0, 0x14, 0x60, 5),
	PIN(8, 0, 0x14, 0x60, 4),
	PIN(9, 0, 0x14, 0x60, 3),
	PIN(10, 0, 0x14, 0x60, 2),
	PIN(11, 1, 0x11, 0x60, 0),
	PIN(12, 1, 0x11, 0x60, 18),
	PIN(13, 0, 0x11, 0x70, 0),
	PIN(14, 0, 0x11, 0x70, 1),
	PIN(15, 0, 0x15, 0x40, 4),
	PIN(16, 0, 0x15, 0x40, 5),
	PIN(17, 0, 0x15, 0x40, 0),
	PIN(18, 0, 0x15, 0x40, 1),
	PIN(19, 1, 0x14, 0x50, 2),
	PIN(20, 1, 0x14, 0x50, 1),
	PIN(21, 1, 0x13, 0x70, 17),
	PIN(22, 1, 0x13, 0x70, 23),
	PIN(23, 1, 0x13, 0x70, 20),
	PIN(24, 1, 0x13, 0x70, 19),
	PIN(25, 1, 0x13, 0x70, 21),
	PIN(26, 1, 0x13, 0x70, 22),
	PIN(27, 1, 0x13, 0x70, 18),
	PIN(28, 1, 0x13, 0x70, 25),
	PIN(29, 1, 0x13, 0x70, 26),
	PIN(30, 1, 0x13, 0x70, 27),
	PIN(31, 1, 0x13, 0x70, 24),
	PIN(32, 1, 0x13, 0x70, 28),
	PIN(33, 1, 0x13, 0x80, 0),
	PIN(34, 1, 0x13, 0x70, 31),
	PIN(35, 1, 0x13, 0x70, 29),
	PIN(36, 1, 0x13, 0x70, 30),
	PIN(37, 1, 0x13, 0x80, 1),
	PIN(38, 1, 0x13, 0x70, 11),
	PIN(39, 1, 0x13, 0x70, 10),
	PIN(40, 1, 0x13, 0x70, 0),
	PIN(41, 1, 0x13, 0x70, 1),
	PIN(42, 1, 0x13, 0x70, 9),
	PIN(43, 1, 0x13, 0x70, 8),
	PIN(44, 1, 0x13, 0x70, 7),
	PIN(45, 1, 0x13, 0x70, 6),
	PIN(46, 1, 0x13, 0x70, 5),
	PIN(47, 1, 0x13, 0x70, 4),
	PIN(48, 1, 0x13, 0x70, 3),
	PIN(49, 1, 0x13, 0x70, 2),
	PIN(50, 1, 0x13, 0x70, 15),
	PIN(51, 1, 0x13, 0x70, 12),
	PIN(52, 1, 0x13, 0x70, 13),
	PIN(53, 1, 0x13, 0x70, 14),
	PIN(54, 1, 0x13, 0x70, 16),
	PIN(55, 1, 0x11, 0x60, 12),
	PIN(56, 1, 0x11, 0x60, 13),
	PIN(57, 1, 0x11, 0x60, 11),
	PIN(58, 1, 0x11, 0x60, 2),
	PIN(59, 1, 0x11, 0x60, 3),
	PIN(60, 1, 0x11, 0x60, 4),
	PIN(61, 1, 0x11, 0x60, 1),
	PIN(62, 1, 0x11, 0x60, 5),
	PIN(63, 0, 0x11, 0x70, 2),
	PIN(64, 1, 0x11, 0x60, 6),
	PIN(65, 1, 0x11, 0x60, 7),
	PIN(66, 1, 0x11, 0x60, 8),
	PIN(67, 1, 0x11, 0x60, 9),
	PIN(68, 1, 0x11, 0x60, 10),
	PIN(69, 1, 0x15, 0x50, 1),
	PIN(70, 1, 0x15, 0x50, 2),
	PIN(71, 0, 0x15, 0x40, 2),
	PIN(72, 0, 0x15, 0x40, 3),
	PIN(73, 1, 0x14, 0x50, 3),
	PIN(74, 1, 0x14, 0x50, 0),
	PIN(75, 0, 0x14, 0x60, 7),
	PIN(76, 0, 0x14, 0x60, 6),
	PIN(77, 0, 0x14, 0x60, 1),
	PIN(78, 0, 0x14, 0x60, 0),
	PIN(79, 0, 0x14, 0x60, 8),
	PIN(80, 1, 0x11, 0x60, 16),
	PIN(81, 1, 0x11, 0x60, 17),
	PIN(82, 1, 0x11, 0x60, 14),
	PIN(83, 1, 0x11, 0x60, 15),
};

void mtk_pin_init(void);
void mt7988_set_default_pinmux(void);
void mt_set_gpio_dir(int gpio, int direction);
int mt_get_gpio_dir(int gpio);
void mt_set_gpio_pull(int gpio, int pull);
int mt_get_gpio_pull(int gpio);
void mt_set_gpio_out(int gpio, int value);
int mt_get_gpio_in(int gpio);
void mt_set_pinmux_mode(int gpio, int mode);
int mt_get_pinmux_mode(int gpio);

#endif /* MT_GPIO_H */
