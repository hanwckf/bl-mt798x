/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/gpio.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include "pinctrl.h"

#define MAX_GPIO_PIN		103
#define MAX_GPIO_MODE		8
#define MAX_GPIO_DIR		2
#define MAX_GPIO_OUT		2

/**
 * struct mtk_pin_info - For all pins' setting.
 * @pin: The pin number.
 * @offset: The offset of pin setting register.
 * @bit: The offset of setting value register.
 * @width: The width of setting bit.
 * @ip_num: The IP number of setting register use.
 */
struct mtk_pin_info {
	uint32_t pin;
	uint32_t offset;
	uint8_t bit;
	uint8_t width;
	uint8_t ip_num;
};
#define MTK_PIN_INFO(_pin, _offset, _bit, _width, _ip_num)	\
{								\
	.pin = _pin,						\
	.offset = _offset,					\
	.bit = _bit,						\
	.width = _width,					\
	.ip_num = _ip_num,					\
}

static const struct mtk_pin_info mtk_pin_info_mode[] = {
	MTK_PIN_INFO(0, 0x320, 16, 4, 0),
	MTK_PIN_INFO(1, 0x3A0, 16, 4, 0),
	MTK_PIN_INFO(2, 0x3A0, 20, 4, 0),
	MTK_PIN_INFO(3, 0x3A0, 24, 4, 0),
	MTK_PIN_INFO(4, 0x3A0, 28, 4, 0),
	MTK_PIN_INFO(5, 0x320, 0, 4, 0),
	MTK_PIN_INFO(6, 0x300, 4, 4, 0),
	MTK_PIN_INFO(7, 0x300, 4, 4, 0),
	MTK_PIN_INFO(8, 0x350, 20, 4, 0),
	MTK_PIN_INFO(9, 0x350, 24, 4, 0),
	MTK_PIN_INFO(10, 0x300, 8, 4, 0),
	MTK_PIN_INFO(11, 0x300, 8, 4, 0),
	MTK_PIN_INFO(12, 0x300, 8, 4, 0),
	MTK_PIN_INFO(13, 0x300, 8, 4, 0),
	MTK_PIN_INFO(14, 0x320, 4, 4, 0),
	MTK_PIN_INFO(15, 0x320, 8, 4, 0),
	MTK_PIN_INFO(16, 0x320, 20, 4, 0),
	MTK_PIN_INFO(17, 0x320, 24, 4, 0),
	MTK_PIN_INFO(18, 0x310, 16, 4, 0),
	MTK_PIN_INFO(19, 0x310, 20, 4, 0),
	MTK_PIN_INFO(20, 0x310, 24, 4, 0),
	MTK_PIN_INFO(21, 0x310, 28, 4, 0),
	MTK_PIN_INFO(22, 0x380, 16, 4, 0),
	MTK_PIN_INFO(23, 0x300, 24, 4, 0),
	MTK_PIN_INFO(24, 0x300, 24, 4, 0),
	MTK_PIN_INFO(25, 0x300, 12, 4, 0),
	MTK_PIN_INFO(26, 0x300, 12, 4, 0),
	MTK_PIN_INFO(27, 0x300, 12, 4, 0),
	MTK_PIN_INFO(28, 0x300, 12, 4, 0),
	MTK_PIN_INFO(29, 0x300, 12, 4, 0),
	MTK_PIN_INFO(30, 0x300, 12, 4, 0),
	MTK_PIN_INFO(31, 0x300, 12, 4, 0),
	MTK_PIN_INFO(32, 0x300, 12, 4, 0),
	MTK_PIN_INFO(33, 0x300, 12, 4, 0),
	MTK_PIN_INFO(34, 0x300, 12, 4, 0),
	MTK_PIN_INFO(35, 0x300, 12, 4, 0),
	MTK_PIN_INFO(36, 0x300, 12, 4, 0),
	MTK_PIN_INFO(37, 0x300, 20, 4, 0),
	MTK_PIN_INFO(38, 0x300, 20, 4, 0),
	MTK_PIN_INFO(39, 0x300, 20, 4, 0),
	MTK_PIN_INFO(40, 0x300, 20, 4, 0),
	MTK_PIN_INFO(41, 0x300, 20, 4, 0),
	MTK_PIN_INFO(42, 0x300, 20, 4, 0),
	MTK_PIN_INFO(43, 0x300, 20, 4, 0),
	MTK_PIN_INFO(44, 0x300, 20, 4, 0),
	MTK_PIN_INFO(45, 0x300, 20, 4, 0),
	MTK_PIN_INFO(46, 0x300, 20, 4, 0),
	MTK_PIN_INFO(47, 0x300, 20, 4, 0),
	MTK_PIN_INFO(48, 0x300, 20, 4, 0),
	MTK_PIN_INFO(49, 0x300, 20, 4, 0),
	MTK_PIN_INFO(50, 0x300, 20, 4, 0),
	MTK_PIN_INFO(51, 0x330, 4, 4, 0),
	MTK_PIN_INFO(52, 0x330, 8, 4, 0),
	MTK_PIN_INFO(53, 0x330, 12, 4, 0),
	MTK_PIN_INFO(54, 0x330, 16, 4, 0),
	MTK_PIN_INFO(55, 0x330, 20, 4, 0),
	MTK_PIN_INFO(56, 0x330, 24, 4, 0),
	MTK_PIN_INFO(57, 0x330, 28, 4, 0),
	MTK_PIN_INFO(58, 0x340, 0, 4, 0),
	MTK_PIN_INFO(59, 0x340, 4, 4, 0),
	MTK_PIN_INFO(60, 0x340, 8, 4, 0),
	MTK_PIN_INFO(61, 0x340, 12, 4, 0),
	MTK_PIN_INFO(62, 0x340, 16, 4, 0),
	MTK_PIN_INFO(63, 0x340, 20, 4, 0),
	MTK_PIN_INFO(64, 0x340, 24, 4, 0),
	MTK_PIN_INFO(65, 0x340, 28, 4, 0),
	MTK_PIN_INFO(66, 0x350, 0, 4, 0),
	MTK_PIN_INFO(67, 0x350, 4, 4, 0),
	MTK_PIN_INFO(68, 0x350, 8, 4, 0),
	MTK_PIN_INFO(69, 0x350, 12, 4, 0),
	MTK_PIN_INFO(70, 0x350, 16, 4, 0),
	MTK_PIN_INFO(71, 0x300, 16, 4, 0),
	MTK_PIN_INFO(72, 0x300, 16, 4, 0),
	MTK_PIN_INFO(73, 0x310, 0, 4, 0),
	MTK_PIN_INFO(74, 0x310, 4, 4, 0),
	MTK_PIN_INFO(75, 0x310, 8, 4, 0),
	MTK_PIN_INFO(76, 0x310, 12, 4, 0),
	MTK_PIN_INFO(77, 0x320, 28, 4, 0),
	MTK_PIN_INFO(78, 0x320, 12, 4, 0),
	MTK_PIN_INFO(79, 0x3A0, 0, 4, 0),
	MTK_PIN_INFO(80, 0x3A0, 4, 4, 0),
	MTK_PIN_INFO(81, 0x3A0, 8, 4, 0),
	MTK_PIN_INFO(82, 0x3A0, 12, 4, 0),
	MTK_PIN_INFO(83, 0x350, 28, 4, 0),
	MTK_PIN_INFO(84, 0x330, 0, 4, 0),
	MTK_PIN_INFO(85, 0x360, 4, 4, 0),
	MTK_PIN_INFO(86, 0x360, 8, 4, 0),
	MTK_PIN_INFO(87, 0x360, 12, 4, 0),
	MTK_PIN_INFO(88, 0x360, 16, 4, 0),
	MTK_PIN_INFO(89, 0x360, 20, 4, 0),
	MTK_PIN_INFO(90, 0x360, 24, 4, 0),
	MTK_PIN_INFO(91, 0x390, 16, 4, 0),
	MTK_PIN_INFO(92, 0x390, 20, 4, 0),
	MTK_PIN_INFO(93, 0x390, 24, 4, 0),
	MTK_PIN_INFO(94, 0x390, 28, 4, 0),
	MTK_PIN_INFO(95, 0x380, 20, 4, 0),
	MTK_PIN_INFO(96, 0x380, 24, 4, 0),
	MTK_PIN_INFO(97, 0x380, 28, 4, 0),
	MTK_PIN_INFO(98, 0x390, 0, 4, 0),
	MTK_PIN_INFO(99, 0x390, 4, 4, 0),
	MTK_PIN_INFO(100, 0x390, 8, 4, 0),
	MTK_PIN_INFO(101, 0x390, 12, 4, 0),
	MTK_PIN_INFO(102, 0x360, 0, 4, 0),
};

static const struct mtk_pin_info mtk_pin_info_dir[] = {
	MTK_PIN_INFO(0, 0x000, 0, 1, 0),
	MTK_PIN_INFO(1, 0x000, 1, 1, 0),
	MTK_PIN_INFO(2, 0x000, 2, 1, 0),
	MTK_PIN_INFO(3, 0x000, 3, 1, 0),
	MTK_PIN_INFO(4, 0x000, 4, 1, 0),
	MTK_PIN_INFO(5, 0x000, 5, 1, 0),
	MTK_PIN_INFO(6, 0x000, 6, 1, 0),
	MTK_PIN_INFO(7, 0x000, 7, 1, 0),
	MTK_PIN_INFO(8, 0x000, 8, 1, 0),
	MTK_PIN_INFO(9, 0x000, 9, 1, 0),
	MTK_PIN_INFO(10, 0x000, 10, 1, 0),
	MTK_PIN_INFO(11, 0x000, 11, 1, 0),
	MTK_PIN_INFO(12, 0x000, 12, 1, 0),
	MTK_PIN_INFO(13, 0x000, 13, 1, 0),
	MTK_PIN_INFO(14, 0x000, 14, 1, 0),
	MTK_PIN_INFO(15, 0x000, 15, 1, 0),
	MTK_PIN_INFO(16, 0x000, 16, 1, 0),
	MTK_PIN_INFO(17, 0x000, 17, 1, 0),
	MTK_PIN_INFO(18, 0x000, 18, 1, 0),
	MTK_PIN_INFO(19, 0x000, 19, 1, 0),
	MTK_PIN_INFO(20, 0x000, 20, 1, 0),
	MTK_PIN_INFO(21, 0x000, 21, 1, 0),
	MTK_PIN_INFO(22, 0x000, 22, 1, 0),
	MTK_PIN_INFO(23, 0x000, 23, 1, 0),
	MTK_PIN_INFO(24, 0x000, 24, 1, 0),
	MTK_PIN_INFO(25, 0x000, 25, 1, 0),
	MTK_PIN_INFO(26, 0x000, 26, 1, 0),
	MTK_PIN_INFO(27, 0x000, 27, 1, 0),
	MTK_PIN_INFO(28, 0x000, 28, 1, 0),
	MTK_PIN_INFO(29, 0x000, 29, 1, 0),
	MTK_PIN_INFO(30, 0x000, 30, 1, 0),
	MTK_PIN_INFO(31, 0x000, 31, 1, 0),
	MTK_PIN_INFO(32, 0x010, 0, 1, 0),
	MTK_PIN_INFO(33, 0x010, 1, 1, 0),
	MTK_PIN_INFO(34, 0x010, 2, 1, 0),
	MTK_PIN_INFO(35, 0x010, 3, 1, 0),
	MTK_PIN_INFO(36, 0x010, 4, 1, 0),
	MTK_PIN_INFO(37, 0x010, 5, 1, 0),
	MTK_PIN_INFO(38, 0x010, 6, 1, 0),
	MTK_PIN_INFO(39, 0x010, 7, 1, 0),
	MTK_PIN_INFO(40, 0x010, 8, 1, 0),
	MTK_PIN_INFO(41, 0x010, 9, 1, 0),
	MTK_PIN_INFO(42, 0x010, 10, 1, 0),
	MTK_PIN_INFO(43, 0x010, 11, 1, 0),
	MTK_PIN_INFO(44, 0x010, 12, 1, 0),
	MTK_PIN_INFO(45, 0x010, 13, 1, 0),
	MTK_PIN_INFO(46, 0x010, 14, 1, 0),
	MTK_PIN_INFO(47, 0x010, 15, 1, 0),
	MTK_PIN_INFO(48, 0x010, 16, 1, 0),
	MTK_PIN_INFO(49, 0x010, 17, 1, 0),
	MTK_PIN_INFO(50, 0x010, 18, 1, 0),
	MTK_PIN_INFO(51, 0x010, 19, 1, 0),
	MTK_PIN_INFO(52, 0x010, 20, 1, 0),
	MTK_PIN_INFO(53, 0x010, 21, 1, 0),
	MTK_PIN_INFO(54, 0x010, 22, 1, 0),
	MTK_PIN_INFO(55, 0x010, 23, 1, 0),
	MTK_PIN_INFO(56, 0x010, 24, 1, 0),
	MTK_PIN_INFO(57, 0x010, 25, 1, 0),
	MTK_PIN_INFO(58, 0x010, 26, 1, 0),
	MTK_PIN_INFO(59, 0x010, 27, 1, 0),
	MTK_PIN_INFO(60, 0x010, 28, 1, 0),
	MTK_PIN_INFO(61, 0x010, 29, 1, 0),
	MTK_PIN_INFO(62, 0x010, 30, 1, 0),
	MTK_PIN_INFO(63, 0x010, 31, 1, 0),
	MTK_PIN_INFO(64, 0x020, 0, 1, 0),
	MTK_PIN_INFO(65, 0x020, 1, 1, 0),
	MTK_PIN_INFO(66, 0x020, 2, 1, 0),
	MTK_PIN_INFO(67, 0x020, 3, 1, 0),
	MTK_PIN_INFO(68, 0x020, 4, 1, 0),
	MTK_PIN_INFO(69, 0x020, 5, 1, 0),
	MTK_PIN_INFO(70, 0x020, 6, 1, 0),
	MTK_PIN_INFO(71, 0x020, 7, 1, 0),
	MTK_PIN_INFO(72, 0x020, 8, 1, 0),
	MTK_PIN_INFO(73, 0x020, 9, 1, 0),
	MTK_PIN_INFO(74, 0x020, 10, 1, 0),
	MTK_PIN_INFO(75, 0x020, 11, 1, 0),
	MTK_PIN_INFO(76, 0x020, 12, 1, 0),
	MTK_PIN_INFO(77, 0x020, 13, 1, 0),
	MTK_PIN_INFO(78, 0x020, 14, 1, 0),
	MTK_PIN_INFO(79, 0x020, 15, 1, 0),
	MTK_PIN_INFO(80, 0x020, 16, 1, 0),
	MTK_PIN_INFO(81, 0x020, 17, 1, 0),
	MTK_PIN_INFO(82, 0x020, 18, 1, 0),
	MTK_PIN_INFO(83, 0x020, 19, 1, 0),
	MTK_PIN_INFO(84, 0x020, 20, 1, 0),
	MTK_PIN_INFO(85, 0x020, 21, 1, 0),
	MTK_PIN_INFO(86, 0x020, 22, 1, 0),
	MTK_PIN_INFO(87, 0x020, 23, 1, 0),
	MTK_PIN_INFO(88, 0x020, 24, 1, 0),
	MTK_PIN_INFO(89, 0x020, 25, 1, 0),
	MTK_PIN_INFO(90, 0x020, 26, 1, 0),
	MTK_PIN_INFO(91, 0x020, 27, 1, 0),
	MTK_PIN_INFO(92, 0x020, 28, 1, 0),
	MTK_PIN_INFO(93, 0x020, 29, 1, 0),
	MTK_PIN_INFO(94, 0x020, 30, 1, 0),
	MTK_PIN_INFO(95, 0x020, 31, 1, 0),
	MTK_PIN_INFO(96, 0x030, 0, 1, 0),
	MTK_PIN_INFO(97, 0x030, 1, 1, 0),
	MTK_PIN_INFO(98, 0x030, 2, 1, 0),
	MTK_PIN_INFO(99, 0x030, 3, 1, 0),
	MTK_PIN_INFO(100, 0x030, 4, 1, 0),
	MTK_PIN_INFO(101, 0x030, 5, 1, 0),
	MTK_PIN_INFO(102, 0x030, 6, 1, 0),
};
static const struct mtk_pin_info mtk_pin_info_datain[] = {
	MTK_PIN_INFO(0, 0x200, 0, 1, 0),
	MTK_PIN_INFO(1, 0x200, 1, 1, 0),
	MTK_PIN_INFO(2, 0x200, 2, 1, 0),
	MTK_PIN_INFO(3, 0x200, 3, 1, 0),
	MTK_PIN_INFO(4, 0x200, 4, 1, 0),
	MTK_PIN_INFO(5, 0x200, 5, 1, 0),
	MTK_PIN_INFO(6, 0x200, 6, 1, 0),
	MTK_PIN_INFO(7, 0x200, 7, 1, 0),
	MTK_PIN_INFO(8, 0x200, 8, 1, 0),
	MTK_PIN_INFO(9, 0x200, 9, 1, 0),
	MTK_PIN_INFO(10, 0x200, 10, 1, 0),
	MTK_PIN_INFO(11, 0x200, 11, 1, 0),
	MTK_PIN_INFO(12, 0x200, 12, 1, 0),
	MTK_PIN_INFO(13, 0x200, 13, 1, 0),
	MTK_PIN_INFO(14, 0x200, 14, 1, 0),
	MTK_PIN_INFO(15, 0x200, 15, 1, 0),
	MTK_PIN_INFO(16, 0x200, 16, 1, 0),
	MTK_PIN_INFO(17, 0x200, 17, 1, 0),
	MTK_PIN_INFO(18, 0x200, 18, 1, 0),
	MTK_PIN_INFO(19, 0x200, 19, 1, 0),
	MTK_PIN_INFO(20, 0x200, 20, 1, 0),
	MTK_PIN_INFO(21, 0x200, 21, 1, 0),
	MTK_PIN_INFO(22, 0x200, 22, 1, 0),
	MTK_PIN_INFO(23, 0x200, 23, 1, 0),
	MTK_PIN_INFO(24, 0x200, 24, 1, 0),
	MTK_PIN_INFO(25, 0x200, 25, 1, 0),
	MTK_PIN_INFO(26, 0x200, 26, 1, 0),
	MTK_PIN_INFO(27, 0x200, 27, 1, 0),
	MTK_PIN_INFO(28, 0x200, 28, 1, 0),
	MTK_PIN_INFO(29, 0x200, 29, 1, 0),
	MTK_PIN_INFO(30, 0x200, 30, 1, 0),
	MTK_PIN_INFO(31, 0x200, 31, 1, 0),
	MTK_PIN_INFO(32, 0x210, 0, 1, 0),
	MTK_PIN_INFO(33, 0x210, 1, 1, 0),
	MTK_PIN_INFO(34, 0x210, 2, 1, 0),
	MTK_PIN_INFO(35, 0x210, 3, 1, 0),
	MTK_PIN_INFO(36, 0x210, 4, 1, 0),
	MTK_PIN_INFO(37, 0x210, 5, 1, 0),
	MTK_PIN_INFO(38, 0x210, 6, 1, 0),
	MTK_PIN_INFO(39, 0x210, 7, 1, 0),
	MTK_PIN_INFO(40, 0x210, 8, 1, 0),
	MTK_PIN_INFO(41, 0x210, 9, 1, 0),
	MTK_PIN_INFO(42, 0x210, 10, 1, 0),
	MTK_PIN_INFO(43, 0x210, 11, 1, 0),
	MTK_PIN_INFO(44, 0x210, 12, 1, 0),
	MTK_PIN_INFO(45, 0x210, 13, 1, 0),
	MTK_PIN_INFO(46, 0x210, 14, 1, 0),
	MTK_PIN_INFO(47, 0x210, 15, 1, 0),
	MTK_PIN_INFO(48, 0x210, 16, 1, 0),
	MTK_PIN_INFO(49, 0x210, 17, 1, 0),
	MTK_PIN_INFO(50, 0x210, 18, 1, 0),
	MTK_PIN_INFO(51, 0x210, 19, 1, 0),
	MTK_PIN_INFO(52, 0x210, 20, 1, 0),
	MTK_PIN_INFO(53, 0x210, 21, 1, 0),
	MTK_PIN_INFO(54, 0x210, 22, 1, 0),
	MTK_PIN_INFO(55, 0x210, 23, 1, 0),
	MTK_PIN_INFO(56, 0x210, 24, 1, 0),
	MTK_PIN_INFO(57, 0x210, 25, 1, 0),
	MTK_PIN_INFO(58, 0x210, 26, 1, 0),
	MTK_PIN_INFO(59, 0x210, 27, 1, 0),
	MTK_PIN_INFO(60, 0x210, 28, 1, 0),
	MTK_PIN_INFO(61, 0x210, 29, 1, 0),
	MTK_PIN_INFO(62, 0x210, 30, 1, 0),
	MTK_PIN_INFO(63, 0x210, 31, 1, 0),
	MTK_PIN_INFO(64, 0x220, 0, 1, 0),
	MTK_PIN_INFO(65, 0x220, 1, 1, 0),
	MTK_PIN_INFO(66, 0x220, 2, 1, 0),
	MTK_PIN_INFO(67, 0x220, 3, 1, 0),
	MTK_PIN_INFO(68, 0x220, 4, 1, 0),
	MTK_PIN_INFO(69, 0x220, 5, 1, 0),
	MTK_PIN_INFO(70, 0x220, 6, 1, 0),
	MTK_PIN_INFO(71, 0x220, 7, 1, 0),
	MTK_PIN_INFO(72, 0x220, 8, 1, 0),
	MTK_PIN_INFO(73, 0x220, 9, 1, 0),
	MTK_PIN_INFO(74, 0x220, 10, 1, 0),
	MTK_PIN_INFO(75, 0x220, 11, 1, 0),
	MTK_PIN_INFO(76, 0x220, 12, 1, 0),
	MTK_PIN_INFO(77, 0x220, 13, 1, 0),
	MTK_PIN_INFO(78, 0x220, 14, 1, 0),
	MTK_PIN_INFO(79, 0x220, 15, 1, 0),
	MTK_PIN_INFO(80, 0x220, 16, 1, 0),
	MTK_PIN_INFO(81, 0x220, 17, 1, 0),
	MTK_PIN_INFO(82, 0x220, 18, 1, 0),
	MTK_PIN_INFO(83, 0x220, 19, 1, 0),
	MTK_PIN_INFO(84, 0x220, 20, 1, 0),
	MTK_PIN_INFO(85, 0x220, 21, 1, 0),
	MTK_PIN_INFO(86, 0x220, 22, 1, 0),
	MTK_PIN_INFO(87, 0x220, 23, 1, 0),
	MTK_PIN_INFO(88, 0x220, 24, 1, 0),
	MTK_PIN_INFO(89, 0x220, 25, 1, 0),
	MTK_PIN_INFO(90, 0x220, 26, 1, 0),
	MTK_PIN_INFO(91, 0x220, 27, 1, 0),
	MTK_PIN_INFO(92, 0x220, 28, 1, 0),
	MTK_PIN_INFO(93, 0x220, 29, 1, 0),
	MTK_PIN_INFO(94, 0x220, 30, 1, 0),
	MTK_PIN_INFO(95, 0x220, 31, 1, 0),
	MTK_PIN_INFO(96, 0x230, 0, 1, 0),
	MTK_PIN_INFO(97, 0x230, 1, 1, 0),
	MTK_PIN_INFO(98, 0x230, 2, 1, 0),
	MTK_PIN_INFO(99, 0x230, 3, 1, 0),
	MTK_PIN_INFO(100, 0x230, 4, 1, 0),
	MTK_PIN_INFO(101, 0x230, 5, 1, 0),
	MTK_PIN_INFO(102, 0x230, 6, 1, 0),
};
static const struct mtk_pin_info mtk_pin_info_dataout[] = {
	MTK_PIN_INFO(0, 0x100, 0, 1, 0),
	MTK_PIN_INFO(1, 0x100, 1, 1, 0),
	MTK_PIN_INFO(2, 0x100, 2, 1, 0),
	MTK_PIN_INFO(3, 0x100, 3, 1, 0),
	MTK_PIN_INFO(4, 0x100, 4, 1, 0),
	MTK_PIN_INFO(5, 0x100, 5, 1, 0),
	MTK_PIN_INFO(6, 0x100, 6, 1, 0),
	MTK_PIN_INFO(7, 0x100, 7, 1, 0),
	MTK_PIN_INFO(8, 0x100, 8, 1, 0),
	MTK_PIN_INFO(9, 0x100, 9, 1, 0),
	MTK_PIN_INFO(10, 0x100, 10, 1, 0),
	MTK_PIN_INFO(11, 0x100, 11, 1, 0),
	MTK_PIN_INFO(12, 0x100, 12, 1, 0),
	MTK_PIN_INFO(13, 0x100, 13, 1, 0),
	MTK_PIN_INFO(14, 0x100, 14, 1, 0),
	MTK_PIN_INFO(15, 0x100, 15, 1, 0),
	MTK_PIN_INFO(16, 0x100, 16, 1, 0),
	MTK_PIN_INFO(17, 0x100, 17, 1, 0),
	MTK_PIN_INFO(18, 0x100, 18, 1, 0),
	MTK_PIN_INFO(19, 0x100, 19, 1, 0),
	MTK_PIN_INFO(20, 0x100, 20, 1, 0),
	MTK_PIN_INFO(21, 0x100, 21, 1, 0),
	MTK_PIN_INFO(22, 0x100, 22, 1, 0),
	MTK_PIN_INFO(23, 0x100, 23, 1, 0),
	MTK_PIN_INFO(24, 0x100, 24, 1, 0),
	MTK_PIN_INFO(25, 0x100, 25, 1, 0),
	MTK_PIN_INFO(26, 0x100, 26, 1, 0),
	MTK_PIN_INFO(27, 0x100, 27, 1, 0),
	MTK_PIN_INFO(28, 0x100, 28, 1, 0),
	MTK_PIN_INFO(29, 0x100, 29, 1, 0),
	MTK_PIN_INFO(30, 0x100, 30, 1, 0),
	MTK_PIN_INFO(31, 0x100, 31, 1, 0),
	MTK_PIN_INFO(32, 0x110, 0, 1, 0),
	MTK_PIN_INFO(33, 0x110, 1, 1, 0),
	MTK_PIN_INFO(34, 0x110, 2, 1, 0),
	MTK_PIN_INFO(35, 0x110, 3, 1, 0),
	MTK_PIN_INFO(36, 0x110, 4, 1, 0),
	MTK_PIN_INFO(37, 0x110, 5, 1, 0),
	MTK_PIN_INFO(38, 0x110, 6, 1, 0),
	MTK_PIN_INFO(39, 0x110, 7, 1, 0),
	MTK_PIN_INFO(40, 0x110, 8, 1, 0),
	MTK_PIN_INFO(41, 0x110, 9, 1, 0),
	MTK_PIN_INFO(42, 0x110, 10, 1, 0),
	MTK_PIN_INFO(43, 0x110, 11, 1, 0),
	MTK_PIN_INFO(44, 0x110, 12, 1, 0),
	MTK_PIN_INFO(45, 0x110, 13, 1, 0),
	MTK_PIN_INFO(46, 0x110, 14, 1, 0),
	MTK_PIN_INFO(47, 0x110, 15, 1, 0),
	MTK_PIN_INFO(48, 0x110, 16, 1, 0),
	MTK_PIN_INFO(49, 0x110, 17, 1, 0),
	MTK_PIN_INFO(50, 0x110, 18, 1, 0),
	MTK_PIN_INFO(51, 0x110, 19, 1, 0),
	MTK_PIN_INFO(52, 0x110, 20, 1, 0),
	MTK_PIN_INFO(53, 0x110, 21, 1, 0),
	MTK_PIN_INFO(54, 0x110, 22, 1, 0),
	MTK_PIN_INFO(55, 0x110, 23, 1, 0),
	MTK_PIN_INFO(56, 0x110, 24, 1, 0),
	MTK_PIN_INFO(57, 0x110, 25, 1, 0),
	MTK_PIN_INFO(58, 0x110, 26, 1, 0),
	MTK_PIN_INFO(59, 0x110, 27, 1, 0),
	MTK_PIN_INFO(60, 0x110, 28, 1, 0),
	MTK_PIN_INFO(61, 0x110, 29, 1, 0),
	MTK_PIN_INFO(62, 0x110, 30, 1, 0),
	MTK_PIN_INFO(63, 0x110, 31, 1, 0),
	MTK_PIN_INFO(64, 0x120, 0, 1, 0),
	MTK_PIN_INFO(65, 0x120, 1, 1, 0),
	MTK_PIN_INFO(66, 0x120, 2, 1, 0),
	MTK_PIN_INFO(67, 0x120, 3, 1, 0),
	MTK_PIN_INFO(68, 0x120, 4, 1, 0),
	MTK_PIN_INFO(69, 0x120, 5, 1, 0),
	MTK_PIN_INFO(70, 0x120, 6, 1, 0),
	MTK_PIN_INFO(71, 0x120, 7, 1, 0),
	MTK_PIN_INFO(72, 0x120, 8, 1, 0),
	MTK_PIN_INFO(73, 0x120, 9, 1, 0),
	MTK_PIN_INFO(74, 0x120, 10, 1, 0),
	MTK_PIN_INFO(75, 0x120, 11, 1, 0),
	MTK_PIN_INFO(76, 0x120, 12, 1, 0),
	MTK_PIN_INFO(77, 0x120, 13, 1, 0),
	MTK_PIN_INFO(78, 0x120, 14, 1, 0),
	MTK_PIN_INFO(79, 0x120, 15, 1, 0),
	MTK_PIN_INFO(80, 0x120, 16, 1, 0),
	MTK_PIN_INFO(81, 0x120, 17, 1, 0),
	MTK_PIN_INFO(82, 0x120, 18, 1, 0),
	MTK_PIN_INFO(83, 0x120, 19, 1, 0),
	MTK_PIN_INFO(84, 0x120, 20, 1, 0),
	MTK_PIN_INFO(85, 0x120, 21, 1, 0),
	MTK_PIN_INFO(86, 0x120, 22, 1, 0),
	MTK_PIN_INFO(87, 0x120, 23, 1, 0),
	MTK_PIN_INFO(88, 0x120, 24, 1, 0),
	MTK_PIN_INFO(89, 0x120, 25, 1, 0),
	MTK_PIN_INFO(90, 0x120, 26, 1, 0),
	MTK_PIN_INFO(91, 0x120, 27, 1, 0),
	MTK_PIN_INFO(92, 0x120, 28, 1, 0),
	MTK_PIN_INFO(93, 0x120, 29, 1, 0),
	MTK_PIN_INFO(94, 0x120, 30, 1, 0),
	MTK_PIN_INFO(95, 0x120, 31, 1, 0),
	MTK_PIN_INFO(96, 0x130, 0, 1, 0),
	MTK_PIN_INFO(97, 0x130, 1, 1, 0),
	MTK_PIN_INFO(98, 0x130, 2, 1, 0),
	MTK_PIN_INFO(99, 0x130, 3, 1, 0),
	MTK_PIN_INFO(100, 0x130, 4, 1, 0),
	MTK_PIN_INFO(101, 0x130, 5, 1, 0),
	MTK_PIN_INFO(102, 0x130, 6, 1, 0),
};
static const struct mtk_pin_info mtk_pin_info_pu[] = {
	MTK_PIN_INFO(0, 0x930, 0, 1, 0),
	MTK_PIN_INFO(1, 0x930, 1, 1, 0),
	MTK_PIN_INFO(2, 0x930, 2, 1, 0),
	MTK_PIN_INFO(3, 0x930, 3, 1, 0),
	MTK_PIN_INFO(4, 0x930, 4, 1, 0),
	MTK_PIN_INFO(5, 0x930, 5, 1, 0),
	MTK_PIN_INFO(6, 0x930, 6, 1, 0),
	MTK_PIN_INFO(7, 0x930, 7, 1, 0),
	MTK_PIN_INFO(8, 0x930, 8, 1, 0),
	MTK_PIN_INFO(9, 0x930, 9, 1, 0),
	MTK_PIN_INFO(10, 0x930, 10, 1, 0),
	MTK_PIN_INFO(11, 0x930, 11, 1, 0),
	MTK_PIN_INFO(12, 0x930, 12, 1, 0),
	MTK_PIN_INFO(13, 0x930, 13, 1, 0),
	MTK_PIN_INFO(14, 0x930, 14, 1, 0),
	MTK_PIN_INFO(15, 0x930, 15, 1, 0),
	MTK_PIN_INFO(16, 0x930, 16, 1, 0),
	MTK_PIN_INFO(17, 0x930, 17, 1, 0),
	MTK_PIN_INFO(18, 0x930, 18, 1, 0),
	MTK_PIN_INFO(19, 0x930, 19, 1, 0),
	MTK_PIN_INFO(20, 0x930, 20, 1, 0),
	MTK_PIN_INFO(21, 0x930, 21, 1, 0),
	MTK_PIN_INFO(22, 0x930, 22, 1, 0),
	MTK_PIN_INFO(23, 0x930, 23, 1, 0),
	MTK_PIN_INFO(24, 0x930, 24, 1, 0),
	MTK_PIN_INFO(25, 0x930, 25, 1, 0),
	MTK_PIN_INFO(26, 0x930, 26, 1, 0),
	MTK_PIN_INFO(27, 0x930, 27, 1, 0),
	MTK_PIN_INFO(28, 0x930, 28, 1, 0),
	MTK_PIN_INFO(29, 0x930, 29, 1, 0),
	MTK_PIN_INFO(30, 0x930, 30, 1, 0),
	MTK_PIN_INFO(31, 0x930, 31, 1, 0),
	MTK_PIN_INFO(32, 0xA30, 0, 1, 0),
	MTK_PIN_INFO(33, 0xA30, 1, 1, 0),
	MTK_PIN_INFO(34, 0xA30, 2, 1, 0),
	MTK_PIN_INFO(35, 0xA30, 3, 1, 0),
	MTK_PIN_INFO(36, 0xA30, 4, 1, 0),
	MTK_PIN_INFO(37, 0xA30, 5, 1, 0),
	MTK_PIN_INFO(38, 0xA30, 6, 1, 0),
	MTK_PIN_INFO(39, 0xA30, 7, 1, 0),
	MTK_PIN_INFO(40, 0xA30, 8, 1, 0),
	MTK_PIN_INFO(41, 0xA30, 9, 1, 0),
	MTK_PIN_INFO(42, 0xA30, 10, 1, 0),
	MTK_PIN_INFO(43, 0xA30, 11, 1, 0),
	MTK_PIN_INFO(44, 0xA30, 12, 1, 0),
	MTK_PIN_INFO(45, 0xA30, 13, 1, 0),
	MTK_PIN_INFO(46, 0xA30, 14, 1, 0),
	MTK_PIN_INFO(47, 0xA30, 15, 1, 0),
	MTK_PIN_INFO(48, 0xA30, 16, 1, 0),
	MTK_PIN_INFO(49, 0xA30, 17, 1, 0),
	MTK_PIN_INFO(50, 0xA30, 18, 1, 0),
	MTK_PIN_INFO(51, 0x830, 0, 1, 0),
	MTK_PIN_INFO(52, 0x830, 1, 1, 0),
	MTK_PIN_INFO(53, 0x830, 2, 1, 0),
	MTK_PIN_INFO(54, 0x830, 3, 1, 0),
	MTK_PIN_INFO(55, 0x830, 4, 1, 0),
	MTK_PIN_INFO(56, 0x830, 5, 1, 0),
	MTK_PIN_INFO(57, 0x830, 6, 1, 0),
	MTK_PIN_INFO(58, 0x830, 7, 1, 0),
	MTK_PIN_INFO(59, 0x830, 10, 1, 0),
	MTK_PIN_INFO(60, 0x830, 11, 1, 0),
	MTK_PIN_INFO(61, 0x830, 8, 1, 0),
	MTK_PIN_INFO(62, 0x830, 9, 1, 0),
	MTK_PIN_INFO(63, 0x830, 12, 1, 0),
	MTK_PIN_INFO(64, 0x830, 13, 1, 0),
	MTK_PIN_INFO(65, 0x830, 14, 1, 0),
	MTK_PIN_INFO(66, 0x830, 15, 1, 0),
	MTK_PIN_INFO(67, 0x830, 18, 1, 0),
	MTK_PIN_INFO(68, 0x830, 19, 1, 0),
	MTK_PIN_INFO(69, 0x830, 16, 1, 0),
	MTK_PIN_INFO(70, 0x830, 17, 1, 0),
	MTK_PIN_INFO(71, 0xB30, 0, 1, 0),
	MTK_PIN_INFO(72, 0xB30, 1, 1, 0),
	MTK_PIN_INFO(73, 0xB30, 4, 1, 0),
	MTK_PIN_INFO(74, 0xB30, 5, 1, 0),
	MTK_PIN_INFO(75, 0xB30, 6, 1, 0),
	MTK_PIN_INFO(76, 0xB30, 7, 1, 0),
	MTK_PIN_INFO(77, 0xB30, 8, 1, 0),
	MTK_PIN_INFO(78, 0xB30, 9, 1, 0),
	MTK_PIN_INFO(79, 0xB30, 10, 1, 0),
	MTK_PIN_INFO(80, 0xB30, 11, 1, 0),
	MTK_PIN_INFO(81, 0xB30, 12, 1, 0),
	MTK_PIN_INFO(82, 0xB30, 13, 1, 0),
	MTK_PIN_INFO(83, 0xB30, 14, 1, 0),
	MTK_PIN_INFO(84, 0xB30, 15, 1, 0),
	MTK_PIN_INFO(85, 0xB30, 16, 1, 0),
	MTK_PIN_INFO(86, 0xB30, 17, 1, 0),
	MTK_PIN_INFO(87, 0xC30, 0, 1, 0),
	MTK_PIN_INFO(88, 0xC30, 1, 1, 0),
	MTK_PIN_INFO(89, 0xC30, 2, 1, 0),
	MTK_PIN_INFO(90, 0xC30, 3, 1, 0),
	MTK_PIN_INFO(91, 0xB30, 18, 1, 0),
	MTK_PIN_INFO(92, 0xB30, 19, 1, 0),
	MTK_PIN_INFO(93, 0xB30, 20, 1, 0),
	MTK_PIN_INFO(94, 0xB30, 21, 1, 0),
	MTK_PIN_INFO(95, 0xB30, 22, 1, 0),
	MTK_PIN_INFO(96, 0xB30, 23, 1, 0),
	MTK_PIN_INFO(97, 0xB30, 24, 1, 0),
	MTK_PIN_INFO(98, 0xB30, 25, 1, 0),
	MTK_PIN_INFO(99, 0xB30, 26, 1, 0),
	MTK_PIN_INFO(100, 0xB30, 27, 1, 0),
	MTK_PIN_INFO(101, 0xB30, 28, 1, 0),
	MTK_PIN_INFO(102, 0xB30, 29, 1, 0),
};
static const struct mtk_pin_info mtk_pin_info_pd[] = {
	MTK_PIN_INFO(0, 0x940, 0, 1, 0),
	MTK_PIN_INFO(1, 0x940, 1, 1, 0),
	MTK_PIN_INFO(2, 0x940, 2, 1, 0),
	MTK_PIN_INFO(3, 0x940, 3, 1, 0),
	MTK_PIN_INFO(4, 0x940, 4, 1, 0),
	MTK_PIN_INFO(5, 0x940, 5, 1, 0),
	MTK_PIN_INFO(6, 0x940, 6, 1, 0),
	MTK_PIN_INFO(7, 0x940, 7, 1, 0),
	MTK_PIN_INFO(8, 0x940, 8, 1, 0),
	MTK_PIN_INFO(9, 0x940, 9, 1, 0),
	MTK_PIN_INFO(10, 0x940, 10, 1, 0),
	MTK_PIN_INFO(11, 0x940, 11, 1, 0),
	MTK_PIN_INFO(12, 0x940, 12, 1, 0),
	MTK_PIN_INFO(13, 0x940, 13, 1, 0),
	MTK_PIN_INFO(14, 0x940, 14, 1, 0),
	MTK_PIN_INFO(15, 0x940, 15, 1, 0),
	MTK_PIN_INFO(16, 0x940, 16, 1, 0),
	MTK_PIN_INFO(17, 0x940, 17, 1, 0),
	MTK_PIN_INFO(18, 0x940, 18, 1, 0),
	MTK_PIN_INFO(19, 0x940, 19, 1, 0),
	MTK_PIN_INFO(20, 0x940, 20, 1, 0),
	MTK_PIN_INFO(21, 0x940, 21, 1, 0),
	MTK_PIN_INFO(22, 0x940, 22, 1, 0),
	MTK_PIN_INFO(23, 0x940, 23, 1, 0),
	MTK_PIN_INFO(24, 0x940, 24, 1, 0),
	MTK_PIN_INFO(25, 0x940, 25, 1, 0),
	MTK_PIN_INFO(26, 0x940, 26, 1, 0),
	MTK_PIN_INFO(27, 0x940, 27, 1, 0),
	MTK_PIN_INFO(28, 0x940, 28, 1, 0),
	MTK_PIN_INFO(29, 0x940, 29, 1, 0),
	MTK_PIN_INFO(30, 0x940, 30, 1, 0),
	MTK_PIN_INFO(31, 0x940, 31, 1, 0),
	MTK_PIN_INFO(32, 0xA40, 0, 1, 0),
	MTK_PIN_INFO(33, 0xA40, 1, 1, 0),
	MTK_PIN_INFO(34, 0xA40, 2, 1, 0),
	MTK_PIN_INFO(35, 0xA40, 3, 1, 0),
	MTK_PIN_INFO(36, 0xA40, 4, 1, 0),
	MTK_PIN_INFO(37, 0xA40, 5, 1, 0),
	MTK_PIN_INFO(38, 0xA40, 6, 1, 0),
	MTK_PIN_INFO(39, 0xA40, 7, 1, 0),
	MTK_PIN_INFO(40, 0xA40, 8, 1, 0),
	MTK_PIN_INFO(41, 0xA40, 9, 1, 0),
	MTK_PIN_INFO(42, 0xA40, 10, 1, 0),
	MTK_PIN_INFO(43, 0xA40, 11, 1, 0),
	MTK_PIN_INFO(44, 0xA40, 12, 1, 0),
	MTK_PIN_INFO(45, 0xA40, 13, 1, 0),
	MTK_PIN_INFO(46, 0xA40, 14, 1, 0),
	MTK_PIN_INFO(47, 0xA40, 15, 1, 0),
	MTK_PIN_INFO(48, 0xA40, 16, 1, 0),
	MTK_PIN_INFO(49, 0xA40, 17, 1, 0),
	MTK_PIN_INFO(50, 0xA40, 18, 1, 0),
	MTK_PIN_INFO(51, 0x840, 0, 1, 0),
	MTK_PIN_INFO(52, 0x840, 1, 1, 0),
	MTK_PIN_INFO(53, 0x840, 2, 1, 0),
	MTK_PIN_INFO(54, 0x840, 3, 1, 0),
	MTK_PIN_INFO(55, 0x840, 4, 1, 0),
	MTK_PIN_INFO(56, 0x840, 5, 1, 0),
	MTK_PIN_INFO(57, 0x840, 6, 1, 0),
	MTK_PIN_INFO(58, 0x840, 7, 1, 0),
	MTK_PIN_INFO(59, 0x840, 10, 1, 0),
	MTK_PIN_INFO(60, 0x840, 11, 1, 0),
	MTK_PIN_INFO(61, 0x840, 8, 1, 0),
	MTK_PIN_INFO(62, 0x840, 9, 1, 0),
	MTK_PIN_INFO(63, 0x840, 12, 1, 0),
	MTK_PIN_INFO(64, 0x840, 13, 1, 0),
	MTK_PIN_INFO(65, 0x840, 14, 1, 0),
	MTK_PIN_INFO(66, 0x840, 15, 1, 0),
	MTK_PIN_INFO(67, 0x840, 18, 1, 0),
	MTK_PIN_INFO(68, 0x840, 19, 1, 0),
	MTK_PIN_INFO(69, 0x840, 16, 1, 0),
	MTK_PIN_INFO(70, 0x840, 17, 1, 0),
	MTK_PIN_INFO(71, 0xB40, 0, 1, 0),
	MTK_PIN_INFO(72, 0xB40, 1, 1, 0),
	MTK_PIN_INFO(73, 0xB40, 4, 1, 0),
	MTK_PIN_INFO(74, 0xB40, 5, 1, 0),
	MTK_PIN_INFO(75, 0xB40, 6, 1, 0),
	MTK_PIN_INFO(76, 0xB40, 7, 1, 0),
	MTK_PIN_INFO(77, 0xB40, 8, 1, 0),
	MTK_PIN_INFO(78, 0xB40, 9, 1, 0),
	MTK_PIN_INFO(79, 0xB40, 10, 1, 0),
	MTK_PIN_INFO(80, 0xB40, 11, 1, 0),
	MTK_PIN_INFO(81, 0xB40, 12, 1, 0),
	MTK_PIN_INFO(82, 0xB40, 13, 1, 0),
	MTK_PIN_INFO(83, 0xB40, 14, 1, 0),
	MTK_PIN_INFO(84, 0xB40, 15, 1, 0),
	MTK_PIN_INFO(85, 0xB40, 16, 1, 0),
	MTK_PIN_INFO(86, 0xB40, 17, 1, 0),
	MTK_PIN_INFO(87, 0xC40, 0, 1, 0),
	MTK_PIN_INFO(88, 0xC40, 1, 1, 0),
	MTK_PIN_INFO(89, 0xC40, 2, 1, 0),
	MTK_PIN_INFO(90, 0xC40, 3, 1, 0),
	MTK_PIN_INFO(91, 0xB40, 18, 1, 0),
	MTK_PIN_INFO(92, 0xB40, 19, 1, 0),
	MTK_PIN_INFO(93, 0xB40, 20, 1, 0),
	MTK_PIN_INFO(94, 0xB40, 21, 1, 0),
	MTK_PIN_INFO(95, 0xB40, 22, 1, 0),
	MTK_PIN_INFO(96, 0xB40, 23, 1, 0),
	MTK_PIN_INFO(97, 0xB40, 24, 1, 0),
	MTK_PIN_INFO(98, 0xB40, 25, 1, 0),
	MTK_PIN_INFO(99, 0xB40, 26, 1, 0),
	MTK_PIN_INFO(100, 0xB40, 27, 1, 0),
	MTK_PIN_INFO(101, 0xB40, 28, 1, 0),
	MTK_PIN_INFO(102, 0xB40, 29, 1, 0),
};

static const
struct mtk_pin_info *to_pin_array(uint32_t pin, uint32_t size,
				   const struct mtk_pin_info pin_info[])
{
	uint32_t i;

	for (i = 0; i < size; i++) {
		if (pin == pin_info[i].pin)
			return &pin_info[i];
	}

	return NULL;
}

static void mtk_set_pin_value(uint32_t pin, uint32_t val, uint32_t size,
			      const struct mtk_pin_info pin_info[])
{
	const struct mtk_pin_info *info;
	uint32_t set_addr, rst_addr;

	info = to_pin_array(pin, size, pin_info);

	set_addr = info->offset + 4;
	rst_addr = info->offset + 8;

	if (val)
		mmio_write_32(GPIO_BASE + set_addr, 1L << info->bit);
	else
		mmio_write_32(GPIO_BASE + rst_addr, 1L << info->bit);
}

static uint32_t mtk_get_pin_value(uint32_t pin, uint32_t size,
				  const struct mtk_pin_info pin_info[])
{
	const struct mtk_pin_info *info;
	uint32_t val;

	info = to_pin_array(pin, size, pin_info);
	val = mmio_read_32(GPIO_BASE + info->offset);

	return ((val >> info->bit) & ((1 << info->width) - 1));
}

static void mtk_update_pin_value(uint32_t pin, uint8_t bit, uint32_t size,
				 const struct mtk_pin_info pin_info[])
{
	uint32_t val, mask;
	const struct mtk_pin_info *info;

	info = to_pin_array(pin, size, pin_info);

	mask = ((1 << info->width) - 1) << info->bit;

	val = mmio_read_32(GPIO_BASE + info->offset);
	val &= ~(mask);
	val |= (bit << info->bit);
	mmio_write_32(GPIO_BASE + info->offset, val);
}

static void mtk_set_pin_dir_chip(uint32_t pin, uint32_t dir)
{
	assert(pin < MAX_GPIO_PIN);
	assert(dir < MAX_GPIO_DIR);

	mtk_set_pin_value(pin, dir, MAX_GPIO_PIN, mtk_pin_info_dir);
}

static int mtk_get_pin_dir_chip(unsigned long pin)
{
    assert(pin < MAX_GPIO_PIN);

    return mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_dir);
}

static void mtk_set_pin_pull_select_chip(uint32_t pin, uint32_t select)
{
	assert(pin < MAX_GPIO_PIN);

	if (select == 1) {
		mtk_set_pin_value(pin, 0, MAX_GPIO_PIN, mtk_pin_info_pd);
		mtk_set_pin_value(pin, 1, MAX_GPIO_PIN, mtk_pin_info_pu);
	} else {
		mtk_set_pin_value(pin, 0, MAX_GPIO_PIN, mtk_pin_info_pu);
		mtk_set_pin_value(pin, 1, MAX_GPIO_PIN, mtk_pin_info_pd);
    }
}

static int mtk_get_pin_pull_select_chip(uint32_t pin)
{
	uint32_t bit_pu, bit_pd, pull_sel;

	bit_pu = mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_pu);
	bit_pd = mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_pd);
	pull_sel = (bit_pd | bit_pu << 1);

	if (pull_sel == 0x02)
		pull_sel = 1;
	else if (pull_sel == 0x01)
		pull_sel = 0;
	else
		return -1;

	return pull_sel;
}

static void mtk_set_pin_out_chip(uint32_t pin, uint32_t out)
{
	assert(pin < MAX_GPIO_PIN);
	assert(out < MAX_GPIO_OUT);

	mtk_set_pin_value(pin, out, MAX_GPIO_PIN, mtk_pin_info_dataout);
}

static int mtk_get_pin_in_chip(uint32_t pin)
{
	assert(pin < MAX_GPIO_PIN);

	return mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_datain);
}

static void mtk_set_pin_mode_chip(uint32_t pin, uint32_t mode)
{
	assert(pin < MAX_GPIO_PIN);
	assert(mode < MAX_GPIO_MODE);

	mtk_update_pin_value(pin, mode, MAX_GPIO_PIN, mtk_pin_info_mode);
}

static int mtk_get_pin_mode_chip(uint32_t pin)
{
    assert(pin < MAX_GPIO_PIN);

    return mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_mode);
}

static void mtk_set_pin_pull(int gpio, int pull)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	mtk_set_pin_pull_select_chip(pin, pull);
}

static int mtk_get_pin_pull(int gpio)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	return mtk_get_pin_pull_select_chip(pin);
}

static void mtk_set_pin_out(int gpio, int value)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	mtk_set_pin_out_chip(pin, value);
}

static int mtk_get_pin_in(int gpio)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	return mtk_get_pin_in_chip(pin);
}

void mtk_set_pin_mode(int gpio, int mode)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	mtk_set_pin_mode_chip(pin, mode);
}

int mtk_get_pin_mode(int gpio)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	return mtk_get_pin_mode_chip(pin);
}

static void mtk_set_pin_dir(int gpio, int direction)
{
	mtk_set_pin_dir_chip((uint32_t)gpio, direction);
}

static int mtk_get_pin_dir(int gpio)
{
	uint32_t pin;

	pin = (uint32_t)gpio;
	return mtk_get_pin_dir_chip(pin);
}

static const gpio_ops_t mtk_pin_ops = {
	 .get_direction = mtk_get_pin_dir,
	 .set_direction = mtk_set_pin_dir,
	 .get_value = mtk_get_pin_in,
	 .set_value = mtk_set_pin_out,
	 .set_pull = mtk_set_pin_pull,
	 .get_pull = mtk_get_pin_pull,
};

void mtk_pin_init(void)
{
	gpio_init(&mtk_pin_ops);
}
