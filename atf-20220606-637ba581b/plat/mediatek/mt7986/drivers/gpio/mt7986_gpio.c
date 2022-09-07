/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <stdbool.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/gpio.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <mt7986_gpio.h>
#include <platform_def.h>

/* Basic Mode/Direction/Value registers */
#define GPIO_DIR_BASE		(GPIO_BASE + 0x000)
#define GPIO_DOUT_BASE		(GPIO_BASE + 0x100)
#define GPIO_DIN_BASE		(GPIO_BASE + 0x200)
#define GPIO_MODE_BASE		(GPIO_BASE + 0x300)

#define SET_OFFS		0x4
#define CLR_OFFS		0x8

#define GPIO_MODE_BITS		4
#define GPIO_MODE_PINS_PER_REG	8
#define GPIO_MODE_MASK		((1 << GPIO_MODE_BITS) - 1)
#define GPIO_MODE_GET(val, bit)	(((val) >> (GPIO_MODE_BITS * (bit))) & \
				 GPIO_MODE_MASK)
#define GPIO_MODE_SET(val, bit)	(((val) & GPIO_MODE_MASK) << \
				 (GPIO_MODE_BITS * (bit)))

#define GPIO_MODE_REG(n)	(GPIO_MODE_BASE + 0x10 * (n))
#define GPIO_MODE_SET_REG(n)	(GPIO_MODE_BASE + 0x10 * (n) + SET_OFFS)
#define GPIO_MODE_CLR_REG(n)	(GPIO_MODE_BASE + 0x10 * (n) + CLR_OFFS)

#define GPIO_REG_BITS		32
#define GPIO_DIR_REG(n)		(GPIO_DIR_BASE + 0x10 * (n))
#define GPIO_DIR_SET_REG(n)	(GPIO_DIR_BASE + 0x10 * (n) + SET_OFFS)
#define GPIO_DIR_CLR_REG(n)	(GPIO_DIR_BASE + 0x10 * (n) + CLR_OFFS)
#define GPIO_OUT_REG(n)		(GPIO_DOUT_BASE + 0x10 * (n))
#define GPIO_OUT_SET_REG(n)	(GPIO_DOUT_BASE + 0x10 * (n) + SET_OFFS)
#define GPIO_OUT_CLR_REG(n)	(GPIO_DOUT_BASE + 0x10 * (n) + CLR_OFFS)
#define GPIO_IN_REG(n)		(GPIO_DIN_BASE + 0x10 * (n))
#define GPIO_IN_SET_REG(n)	(GPIO_DIN_BASE + 0x10 * (n) + SET_OFFS)
#define GPIO_IN_CLR_REG(n)	(GPIO_DIN_BASE + 0x10 * (n) + CLR_OFFS)

/* GPIO pin and register definitions */
struct mt_pin_info {
	uint8_t id;
	uint8_t flag;
	uint8_t bit;
	uint16_t base;
	uint16_t offset;
};

#define PIN(_id, _flag, _bit, _base, _offset) {		\
		.id = _id,				\
		.flag = _flag,				\
		.bit = _bit,				\
		.base = _base,				\
		.offset = _offset,			\
	}

static const struct mt_pin_info mt7986_pin_infos[] = {
	PIN(0, 1, 17, 0x11, 0x60),
	PIN(1, 1, 10, 0x12, 0x30),
	PIN(2, 1, 11, 0x12, 0x30),
	PIN(3, 1, 0, 0x13, 0x40),
	PIN(4, 1, 1, 0x13, 0x40),
	PIN(5, 1, 0, 0x11, 0x60),
	PIN(6, 1, 1, 0x11, 0x60),
	PIN(7, 1, 0, 0x12, 0x30),
	PIN(8, 1, 1, 0x12, 0x30),
	PIN(9, 1, 2, 0x12, 0x30),
	PIN(10, 1, 3, 0x12, 0x30),
	PIN(11, 1, 8, 0x11, 0x60),
	PIN(12, 1, 9, 0x11, 0x60),
	PIN(13, 1, 10, 0x11, 0x60),
	PIN(14, 1, 11, 0x11, 0x60),
	PIN(15, 1, 2, 0x11, 0x60),
	PIN(16, 1, 3, 0x11, 0x60),
	PIN(17, 1, 4, 0x11, 0x60),
	PIN(18, 1, 5, 0x11, 0x60),
	PIN(19, 1, 6, 0x11, 0x60),
	PIN(20, 1, 7, 0x11, 0x60),
	PIN(21, 1, 12, 0x10, 0x40),
	PIN(22, 1, 13, 0x10, 0x40),
	PIN(23, 1, 14, 0x10, 0x40),
	PIN(24, 1, 18, 0x10, 0x40),
	PIN(25, 1, 17, 0x10, 0x40),
	PIN(26, 1, 15, 0x10, 0x40),
	PIN(27, 1, 16, 0x10, 0x40),
	PIN(28, 1, 19, 0x10, 0x40),
	PIN(29, 1, 20, 0x10, 0x40),
	PIN(30, 1, 23, 0x10, 0x40),
	PIN(31, 1, 22, 0x10, 0x40),
	PIN(32, 1, 21, 0x10, 0x40),
	PIN(33, 1, 4, 0x12, 0x30),
	PIN(34, 1, 8, 0x12, 0x30),
	PIN(35, 1, 7, 0x12, 0x30),
	PIN(36, 1, 5, 0x12, 0x30),
	PIN(37, 1, 6, 0x12, 0x30),
	PIN(38, 1, 9, 0x12, 0x30),
	PIN(39, 1, 19, 0x11, 0x60),
	PIN(40, 1, 20, 0x11, 0x60),
	PIN(41, 1, 12, 0x11, 0x60),
	PIN(42, 1, 23, 0x11, 0x60),
	PIN(43, 1, 24, 0x11, 0x60),
	PIN(44, 1, 21, 0x11, 0x60),
	PIN(45, 1, 22, 0x11, 0x60),
	PIN(46, 1, 27, 0x11, 0x60),
	PIN(47, 1, 28, 0x11, 0x60),
	PIN(48, 1, 25, 0x11, 0x60),
	PIN(49, 1, 26, 0x11, 0x60),
	PIN(50, 1, 2, 0x10, 0x40),
	PIN(51, 1, 3, 0x10, 0x40),
	PIN(52, 1, 4, 0x10, 0x40),
	PIN(53, 1, 5, 0x10, 0x40),
	PIN(54, 1, 6, 0x10, 0x40),
	PIN(55, 1, 7, 0x10, 0x40),
	PIN(56, 1, 8, 0x10, 0x40),
	PIN(57, 1, 9, 0x10, 0x40),
	PIN(58, 1, 1, 0x10, 0x40),
	PIN(59, 1, 0, 0x10, 0x40),
	PIN(60, 1, 10, 0x10, 0x40),
	PIN(61, 1, 11, 0x10, 0x40),
	PIN(62, 1, 15, 0x11, 0x60),
	PIN(63, 1, 14, 0x11, 0x60),
	PIN(64, 1, 13, 0x11, 0x60),
	PIN(65, 1, 16, 0x11, 0x60),
	PIN(66, 1, 2, 0x13, 0x40),
	PIN(67, 1, 3, 0x13, 0x40),
	PIN(68, 1, 4, 0x13, 0x40),
	PIN(69, 0, 1, 0x14, 0x50),
	PIN(70, 0, 0, 0x14, 0x50),
	PIN(71, 0, 16, 0x14, 0x50),
	PIN(72, 0, 14, 0x14, 0x50),
	PIN(73, 0, 15, 0x14, 0x50),
	PIN(74, 0, 4, 0x14, 0x50),
	PIN(75, 0, 6, 0x14, 0x50),
	PIN(76, 0, 7, 0x14, 0x50),
	PIN(77, 0, 8, 0x14, 0x50),
	PIN(78, 0, 2, 0x14, 0x50),
	PIN(79, 0, 3, 0x14, 0x50),
	PIN(80, 0, 9, 0x14, 0x50),
	PIN(81, 0, 10, 0x14, 0x50),
	PIN(82, 0, 11, 0x14, 0x50),
	PIN(83, 0, 12, 0x14, 0x50),
	PIN(84, 0, 13, 0x14, 0x50),
	PIN(85, 0, 5, 0x14, 0x50),
	PIN(86, 0, 1, 0x15, 0x50),
	PIN(87, 0, 0, 0x15, 0x50),
	PIN(88, 0, 14, 0x15, 0x50),
	PIN(89, 0, 12, 0x15, 0x50),
	PIN(90, 0, 13, 0x15, 0x50),
	PIN(91, 0, 4, 0x15, 0x50),
	PIN(92, 0, 5, 0x15, 0x50),
	PIN(93, 0, 6, 0x15, 0x50),
	PIN(94, 0, 7, 0x15, 0x50),
	PIN(95, 0, 2, 0x15, 0x50),
	PIN(96, 0, 3, 0x15, 0x50),
	PIN(97, 0, 8, 0x15, 0x50),
	PIN(98, 0, 9, 0x15, 0x50),
	PIN(99, 0, 10, 0x15, 0x50),
	PIN(100, 0, 11, 0x15, 0x50),
};

static void mt_set_pinmux_mode(int gpio, int mode)
{
	uint32_t pos, bit;

	assert(gpio < MAX_GPIO_PIN);

	pos = gpio / GPIO_MODE_PINS_PER_REG;
	bit = gpio % GPIO_MODE_PINS_PER_REG;

	mmio_write_32(GPIO_MODE_CLR_REG(pos),
		      GPIO_MODE_SET(GPIO_MODE_MASK, bit));
	mmio_write_32(GPIO_MODE_SET_REG(pos), GPIO_MODE_SET(mode, bit));
}

static int mt_get_pinmux_mode(int gpio)
{
	uint32_t pos, bit, val;

	assert(gpio < MAX_GPIO_PIN);

	pos = gpio / GPIO_MODE_PINS_PER_REG;
	bit = gpio % GPIO_MODE_PINS_PER_REG;

	val = mmio_read_32(GPIO_MODE_REG(pos));
	return GPIO_MODE_GET(val, bit);
}

static void mt_set_gpio_dir(int gpio, int direction)
{
	uint32_t pos, bit;

	assert(gpio < MAX_GPIO_PIN);

	pos = gpio / GPIO_REG_BITS;
	bit = gpio % GPIO_REG_BITS;

	if (direction == GPIO_DIR_IN)
		mmio_write_32(GPIO_DIR_CLR_REG(pos), BIT(bit));
	else if (direction == GPIO_DIR_OUT)
		mmio_write_32(GPIO_DIR_SET_REG(pos), BIT(bit));
}

static int mt_get_gpio_dir(int gpio)
{
	uint32_t pos, bit, val;

	assert(gpio < MAX_GPIO_PIN);

	pos = gpio / GPIO_REG_BITS;
	bit = gpio % GPIO_REG_BITS;

	val = mmio_read_32(GPIO_DIR_REG(pos));
	return val & BIT(bit) ? GPIO_DIR_OUT : GPIO_DIR_IN;
}

static void mt_set_gpio_out(int gpio, int value)
{
	uint32_t pos, bit;

	assert(gpio < MAX_GPIO_PIN);

	pos = gpio / GPIO_REG_BITS;
	bit = gpio % GPIO_REG_BITS;

	if (!value)
		mmio_write_32(GPIO_OUT_CLR_REG(pos), BIT(bit));
	else
		mmio_write_32(GPIO_OUT_SET_REG(pos), BIT(bit));
}

static int mt_get_gpio_in(int gpio)
{
	uint32_t pos, bit, val;

	assert(gpio < MAX_GPIO_PIN);

	pos = gpio / GPIO_REG_BITS;
	bit = gpio % GPIO_REG_BITS;

	val = mmio_read_32(GPIO_IN_REG(pos));
	return val & BIT(bit) ? GPIO_LEVEL_HIGH : GPIO_LEVEL_LOW;
}

static uintptr_t mt_gpio_find_reg_addr(uint32_t pin)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[pin];

	switch (gpio_info->base & 0x0f) {
	case 0:
		return IOCFG_RT_BASE;
	case 1:
		return IOCFG_RB_BASE;
	case 2:
		return IOCFG_LT_BASE;
	case 3:
		return IOCFG_LB_BASE;
	case 4:
		return IOCFG_TR_BASE;
	case 5:
		return IOCFG_TL_BASE;
	default:
		return 0;
	}
}

static void mt_gpio_set_spec_pupd(uint32_t pin, int enable, bool pull_down)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[pin];
	uint32_t bit = gpio_info->bit;
	uintptr_t reg1, reg2;

	reg1 = mt_gpio_find_reg_addr(pin) + gpio_info->offset;
	reg2 = reg1 + (gpio_info->base & 0xf0);

	if (enable == MT_GPIO_PULL_ENABLE) {
		mmio_write_32(reg2 + SET_OFFS, BIT(bit));

		if (pull_down)
			mmio_write_32(reg1 + SET_OFFS, BIT(bit));
		else
			mmio_write_32(reg1 + CLR_OFFS, BIT(bit));
	} else {
		mmio_write_32(reg2 + CLR_OFFS, BIT(bit));
		mmio_write_32(reg2 + 0x10 + CLR_OFFS, BIT(bit));
	}
}

static void mt_gpio_set_pupd(uint32_t pin, int enable, bool pull_down)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[pin];
	uint32_t bit = gpio_info->bit;
	uintptr_t reg1, reg2;

	reg1 = mt_gpio_find_reg_addr(pin) + gpio_info->offset;
	reg2 = reg1 - (gpio_info->base & 0xf0);

	if (enable == MT_GPIO_PULL_ENABLE) {
		if (pull_down) {
			mmio_write_32(reg1 + CLR_OFFS, BIT(bit));
			mmio_write_32(reg2 + SET_OFFS, BIT(bit));
		} else {
			mmio_write_32(reg2 + CLR_OFFS, BIT(bit));
			mmio_write_32(reg1 + SET_OFFS, BIT(bit));
		}
	} else {
		mmio_write_32(reg1 + CLR_OFFS, BIT(bit));
		mmio_write_32(reg2 + CLR_OFFS, BIT(bit));
	}
}

static void mt_gpio_set_pull_pupd(uint32_t pin, int enable, bool pull_down)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[pin];

	if (gpio_info->flag)
		mt_gpio_set_spec_pupd(pin, enable, pull_down);
	else
		mt_gpio_set_pupd(pin, enable, pull_down);
}

static int mt_gpio_get_spec_pupd(uint32_t pin)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[pin];
	uint32_t bit = gpio_info->bit;
	uintptr_t reg1, reg2;
	uint32_t r0, r1;

	reg1 = mt_gpio_find_reg_addr(pin) + gpio_info->offset;
	reg2 = reg1 + (gpio_info->base & 0xf0);

	r0 = (mmio_read_32(reg2) >> bit) & 1U;
	r1 = (mmio_read_32(reg2 + 0x010) >> bit) & 1U;

	if (r0 == 0U && r1 == 0U) {
		return GPIO_PULL_NONE;
	} else {
		if (mmio_read_32(reg1) & BIT(bit))
			return GPIO_PULL_DOWN;
		else
			return GPIO_PULL_UP;
	}
}

static int mt_gpio_get_pupd(uint32_t pin)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[pin];
	uint32_t bit = gpio_info->bit;
	uintptr_t reg1, reg2;
	uint32_t pu, pd;

	reg1 = mt_gpio_find_reg_addr(pin) + gpio_info->offset;
	reg2 = reg1 - (gpio_info->base & 0xf0);

	pu = (mmio_read_32(reg1) >> bit) & 1;
	pd = (mmio_read_32(reg2) >> bit) & 1;

	if (pu)
		return GPIO_PULL_UP;
	else if (pd)
		return GPIO_PULL_DOWN;

	return GPIO_PULL_NONE;
}

static void mt_set_gpio_pull(int gpio, int pull)
{
	assert(gpio < MAX_GPIO_PIN);

	if (pull == GPIO_PULL_NONE)
		mt_gpio_set_pull_pupd(gpio, MT_GPIO_PULL_DISABLE, true);
	else if (pull == GPIO_PULL_UP)
		mt_gpio_set_pull_pupd(gpio, MT_GPIO_PULL_ENABLE, false);
	else if (pull == GPIO_PULL_DOWN)
		mt_gpio_set_pull_pupd(gpio, MT_GPIO_PULL_ENABLE, true);
}

/* get pull-up or pull-down, regardless of resistor value */
static int mt_get_gpio_pull(int gpio)
{
	const struct mt_pin_info *gpio_info = &mt7986_pin_infos[gpio];

	assert(gpio < MAX_GPIO_PIN);

	if (gpio_info->flag)
		return mt_gpio_get_spec_pupd(gpio);

	return mt_gpio_get_pupd(gpio);
}

static const gpio_ops_t mtgpio_ops = {
	.get_mode = mt_get_pinmux_mode,
	.set_mode = mt_set_pinmux_mode,
	.get_direction = mt_get_gpio_dir,
	.set_direction = mt_set_gpio_dir,
	.get_value = mt_get_gpio_in,
	.set_value = mt_set_gpio_out,
	.set_pull = mt_set_gpio_pull,
	.get_pull = mt_get_gpio_pull,
};

void mtk_pin_init(void)
{
	gpio_init(&mtgpio_ops);
}

void mt7986_set_default_pinmux(void)
{
	/* JTAG */
	mt_set_pinmux_mode(11, 1);
	mt_set_pinmux_mode(12, 1);
	mt_set_pinmux_mode(13, 1);
	mt_set_pinmux_mode(14, 1);
	mt_set_pinmux_mode(15, 1);
}
