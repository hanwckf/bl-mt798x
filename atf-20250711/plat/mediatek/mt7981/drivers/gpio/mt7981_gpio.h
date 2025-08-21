/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7986_GPIO_H
#define MT7986_GPIO_H

#include <stdbool.h>
#include <stdint.h>
#include <plat/common/common_def.h>

/*  Error Code No. */
#define RSUCCESS        0
#define ERACCESS        1
#define ERINVAL         2
#define ERWRAPPER       3
#define MAX_GPIO_PIN    MT_GPIO_BASE_MAX

/* GPIO base address*/
#define GPIO_BASE        (0x11D00000)
#define IOCFG_RT_BASE    (0x11C00000)
#define IOCFG_RM_BASE    (0x11C10000)
#define IOCFG_RB_BASE    (0x11D20000)
#define IOCFG_LB_BASE    (0x11E00000)
#define IOCFG_BL_BASE    (0x11E20000)
#define IOCFG_TM_BASE    (0x11F00000)
#define IOCFG_LT_BASE    (0x11F10000)

/* GPIO mode */
#define GPIO_MODE2       (GPIO_BASE + 0x320)
#define   GPIO_PIN16_S     (0)
#define   GPIO_PIN17_S     (4)
#define   GPIO_PIN18_S     (8)
#define   GPIO_PIN19_S     (12)
#define   GPIO_PIN20_S     (16)
#define   GPIO_PIN21_S     (20)

#define GPIO_MODE3       (GPIO_BASE + 0x330)
#define   GPIO_PIN24_S   (0)
#define   GPIO_PIN25_S   (4)
#define   GPIO_PIN26_S   (8)
#define   GPIO_PIN27_S   (12)
#define   GPIO_PIN28_S   (16)

#define GPIO_MODE4       (GPIO_BASE + 0x340)
#define   GPIO_PIN33_S   (4)
#define   GPIO_PIN34_S   (8)
#define   GPIO_PIN35_S   (12)
#define   GPIO_PIN36_S   (16)
#define   GPIO_PIN37_S   (20)
#define   GPIO_PIN38_S   (24)

/* GPIO driving*/
#define GPIO_RM_DRV_CFG0   (IOCFG_RM_BASE + 0x0)
#define   SPI0_WP_DRV_S    (18)
#define   SPI0_MOSI_DRV_S  (15)
#define   SPI0_MISO_DRV_S  (12)
#define   SPI0_HOLD_DRV_S  (9)
#define   SPI0_CS_DRV_S    (6)
#define   SPI0_CLK_DRV_S   (3)

#define GPIO_LT_DRV_CFG0   (IOCFG_LT_BASE + 0x0)
#define   SPI2_WP_DRV_S    (27)
#define   SPI2_MOSI_DRV_S  (24)
#define   SPI2_MISO_DRV_S  (21)
#define   SPI2_HOLD_DRV_S  (18)
#define   SPI2_CS_DRV_S    (15)
#define   SPI2_CLK_DRV_S   (12)

/* GPIO PU/PD*/
#define GPIO_RM_PUPD_CFG0  (IOCFG_RM_BASE + 0x30)
#define   SPI0_PUPD_S      (1)
#define GPIO_RM_R0_CFG0    (IOCFG_RM_BASE + 0x40)
#define   SPI0_R0_S        (1)
#define GPIO_RM_R1_CFG0    (IOCFG_RM_BASE + 0x50)
#define   SPI0_R1_S        (1)

#define GPIO_LT_PUPD_CFG0  (IOCFG_LT_BASE + 0x30)
#define   SPI2_PUPD_S      (4)
#define GPIO_LT_R0_CFG0    (IOCFG_LT_BASE + 0x40)
#define   SPI2_R0_S        (4)
#define GPIO_LT_R1_CFG0    (IOCFG_LT_BASE + 0x50)
#define   SPI2_R1_S        (4)


#define MSDC_GPIO_DRV_CFG0      (IOCFG_RM_BASE + 0x0)
#define EMMC45_RSTB_DRV_S       0
#define EMMC45_DAT0_DRV_S       3
#define EMMC45_DAT3_DRV_S       6
#define EMMC45_DAT4_DRV_S       9
#define EMMC45_DAT2_DRV_S       12
#define EMMC45_DAT1_DRV_S       15
#define EMMC45_DAT5_DRV_S       18
#define EMMC45_DAT6_DRV_S       21
#define EMMC45_CLK_DRV_S        24
#define EMMC45_CMD_DRV_S        27

#define MSDC_GPIO_DRV_CFG1      (IOCFG_RM_BASE + 0x10)
#define EMMC45_DAT7_DRV_S       0

#define MSDC_GPIO_IES_CFG0      (IOCFG_RM_BASE + 0x20)
#define EMMC45_GPIO_IES_S       0

#define MSDC_GPIO_R0_CFG0       (IOCFG_RM_BASE + 0x40)
#define EMMC45_GPIO_R0_S        0

#define MSDC_GPIO_R1_CFG0       (IOCFG_RM_BASE + 0x50)
#define EMMC45_GPIO_R1_S        0

#define MSDC_GPIO_SMT_CFG0      (IOCFG_RM_BASE + 0x90)
#define EMMC45_GPIO_SMT_S       0

/* Enumeration for GPIO pin */
typedef enum GPIO_PIN {
	GPIO_UNSUPPORTED = -1,

	GPIO0, GPIO1, GPIO2, GPIO3, GPIO4, GPIO5, GPIO6, GPIO7,
	GPIO8, GPIO9, GPIO10, GPIO11, GPIO12, GPIO13, GPIO14, GPIO15,
	GPIO16, GPIO17, GPIO18, GPIO19, GPIO20, GPIO21, GPIO22, GPIO23,
	GPIO24, GPIO25, GPIO26, GPIO27, GPIO28, GPIO29, GPIO30, GPIO31,
	GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, GPIO37, GPIO38, GPIO39,
	GPIO40, GPIO41, GPIO42, GPIO43, GPIO44, GPIO45, GPIO46, GPIO47,
	GPIO48, GPIO49, GPIO50, GPIO51, GPIO52, GPIO53, GPIO54, GPIO55,
	GPIO56, GPIO57, GPIO58, GPIO59, GPIO60, GPIO61, GPIO62, GPIO63,
	GPIO64, GPIO65, GPIO66, GPIO67, GPIO68, GPIO69, GPIO70, GPIO71,
	GPIO72, GPIO73, GPIO74, GPIO75, GPIO76, GPIO77, GPIO78, GPIO79,
	GPIO80, GPIO81, GPIO82, GPIO83, GPIO84, GPIO85, GPIO86, GPIO87,
	GPIO88, GPIO89, GPIO90, GPIO91, GPIO92, GPIO93, GPIO94, GPIO95,
	GPIO96, GPIO97, GPIO98, GPIO99, GPIO100, MT_GPIO_BASE_MAX
} GPIO_PIN;

/* GPIO MODE CONTROL VALUE*/
typedef enum {
	GPIO_MODE_UNSUPPORTED = -1,
	GPIO_MODE_GPIO  = 0,
	GPIO_MODE_00    = 0,
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
	MT_GPIO_DIR_OUT    = 0,
	MT_GPIO_DIR_IN     = 1,
	MT_GPIO_DIR_MAX,
	MT_GPIO_DIR_DEFAULT = MT_GPIO_DIR_IN,
} GPIO_DIR;

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

/* GPIO PULL-UP/PULL-DOWN*/
typedef enum {
	MT_GPIO_PULL_UNSUPPORTED = -1,
	MT_GPIO_PULL_NONE        = 0,
	MT_GPIO_PULL_UP          = 1,
	MT_GPIO_PULL_DOWN        = 2,
	MT_GPIO_PULL_MAX,
	MT_GPIO_PULL_DEFAULT = MT_GPIO_PULL_DOWN
} GPIO_PULL;

/* GPIO OUTPUT */
typedef enum {
	MT_GPIO_OUT_UNSUPPORTED = -1,
	MT_GPIO_OUT_ZERO = 0,
	MT_GPIO_OUT_ONE  = 1,

	MT_GPIO_OUT_MAX,
	MT_GPIO_OUT_DEFAULT = MT_GPIO_OUT_ZERO,
	MT_GPIO_DATA_OUT_DEFAULT = MT_GPIO_OUT_ZERO,  /*compatible with DCT*/
} GPIO_OUT;

/* GPIO INPUT */
typedef enum {
	MT_GPIO_IN_UNSUPPORTED = -1,
	MT_GPIO_IN_ZERO = 0,
	MT_GPIO_IN_ONE  = 1,

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
	VAL_REGS mode[13];
} GPIO_REGS;


#define PIN(_id, _flag, _bit, _base, _offset) {		\
		.id = _id,				\
		.flag = _flag,				\
		.bit = _bit,				\
		.base = _base,				\
		.offset = _offset,			\
	}

struct mt_pin_info {
	uint8_t id;
	uint8_t flag;
	uint8_t bit;
	uint16_t base;
	/* pupd/r0/r1 type is pupd offset,
	 * pu/pd type is pu offsert.
	 */
	uint16_t offset;
};

static const struct mt_pin_info mt7981_pin_infos[] = {
	PIN(0, 1, 1, 0x11, 0x20),
	PIN(1, 1, 0, 0x11, 0x20),

	PIN(2, 1, 6, 0x15, 0x30),

	PIN(3, 1, 6, 0x14, 0x30),
	PIN(4, 1, 2, 0x14, 0x30),
	PIN(5, 1, 1, 0x14, 0x30),
	PIN(6, 1, 3, 0x14, 0x30),
	PIN(7, 1, 0, 0x14, 0x30),
	PIN(8, 1, 4, 0x14, 0x30),
	PIN(9, 1, 9, 0x14, 0x30),

	PIN(10, 1, 8, 0x15, 0x30),
	PIN(11, 1, 10, 0x15, 0x30),
	PIN(12, 1, 7, 0x15, 0x30),
	PIN(13, 1, 11, 0x15, 0x30),

	PIN(14, 1, 9, 0x14, 0x30),

	PIN(15, 1, 0, 0x12, 0x30),
	PIN(16, 1, 1, 0x12, 0x30),
	PIN(17, 1, 5, 0x12, 0x30),
	PIN(18, 1, 4, 0x12, 0x30),
	PIN(19, 1, 2, 0x12, 0x30),
	PIN(20, 1, 3, 0x12, 0x30),
	PIN(21, 1, 6, 0x12, 0x30),
	PIN(22, 1, 7, 0x12, 0x30),
	PIN(23, 1, 10, 0x12, 0x30),
	PIN(24, 1, 9, 0x12, 0x30),
	PIN(25, 1, 8, 0x12, 0x30),

	PIN(26, 1, 0, 0x15, 0x30),
	PIN(27, 1, 4, 0x15, 0x30),
	PIN(28, 1, 3, 0x15, 0x30),
	PIN(29, 1, 1, 0x15, 0x30),
	PIN(30, 1, 2, 0x15, 0x30),
	PIN(31, 1, 5, 0x15, 0x30),

	PIN(32, 1, 2, 0x11, 0x20),
	PIN(33, 1, 3, 0x11, 0x20),

	PIN(34, 1, 5, 0x14, 0x30),
	PIN(35, 1, 7, 0x14, 0x30),

	PIN(36, 1, 2, 0x13, 0x20),
	PIN(37, 1, 3, 0x13, 0x20),
	PIN(38, 1, 0, 0x13, 0x20),
	PIN(39, 1, 1, 0x13, 0x20),

	PIN(40, 0, 1, 0x17, 0x50),
	PIN(41, 0, 0, 0x17, 0x50),
	PIN(42, 0, 9, 0x17, 0x50),
	PIN(43, 0, 7, 0x17, 0x50),
	PIN(44, 0, 8, 0x17, 0x50),
	PIN(45, 0, 3, 0x17, 0x50),
	PIN(46, 0, 4, 0x17, 0x50),
	PIN(47, 0, 5, 0x17, 0x50),
	PIN(48, 0, 6, 0x17, 0x50),
	PIN(49, 0, 2, 0x17, 0x50),
	PIN(50, 0, 0, 0x16, 0x30),
	PIN(51, 0, 2, 0x16, 0x30),
	PIN(52, 0, 3, 0x16, 0x30),
	PIN(53, 0, 4, 0x16, 0x30),
	PIN(54, 0, 5, 0x16, 0x30),
	PIN(55, 0, 6, 0x16, 0x30),
	PIN(56, 0, 1, 0x16, 0x30),
};

void mtk_pin_init(void);
void mt7981_set_default_pinmux(void);
void mt_set_gpio_dir(int gpio, int direction);
int mt_get_gpio_dir(int gpio);
void mt_set_gpio_pull(int gpio, int pull);
int mt_get_gpio_pull(int gpio);
void mt_set_gpio_out(int gpio, int value);
int mt_get_gpio_in(int gpio);
void mt_set_pinmux_mode(int gpio, int mode);
int mt_get_pinmux_mode(int gpio);

#endif /* MT_GPIO_H */
