/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <drivers/gpio.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include "pinctrl.h"

#define MAX_GPIO_PIN		79
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
	MTK_PIN_INFO(0, 0x7300, 0, 4, 0),
	MTK_PIN_INFO(1, 0x7300, 4, 4, 0),
	MTK_PIN_INFO(2, 0x7300, 8, 4, 0),
	MTK_PIN_INFO(3, 0x7300, 12, 4, 0),
	MTK_PIN_INFO(4, 0x7300, 16, 4, 0),
	MTK_PIN_INFO(5, 0x7300, 20, 4, 0),
	MTK_PIN_INFO(6, 0x7300, 24, 4, 0),
	MTK_PIN_INFO(7, 0x7300, 28, 4, 0),
	MTK_PIN_INFO(8, 0x7310, 0, 4, 0),
	MTK_PIN_INFO(9, 0x7310, 4, 4, 0),
	MTK_PIN_INFO(10, 0x7310, 8, 4, 0),
	MTK_PIN_INFO(11, 0x7310, 12, 4, 0),
	MTK_PIN_INFO(12, 0x7310, 16, 4, 0),
	MTK_PIN_INFO(13, 0x7310, 20, 4, 0),
	MTK_PIN_INFO(14, 0x7310, 24, 4, 0),
	MTK_PIN_INFO(15, 0x7310, 28, 4, 0),
	MTK_PIN_INFO(16, 0x7320, 0, 4, 0),
	MTK_PIN_INFO(17, 0x7320, 4, 4, 0),
	MTK_PIN_INFO(18, 0x7320, 8, 4, 0),
	MTK_PIN_INFO(19, 0x7320, 12, 4, 0),
	MTK_PIN_INFO(20, 0x7320, 16, 4, 0),
	MTK_PIN_INFO(21, 0x7320, 20, 4, 0),
	MTK_PIN_INFO(22, 0x7320, 24, 4, 0),
	MTK_PIN_INFO(23, 0x7320, 28, 4, 0),
	MTK_PIN_INFO(24, 0x7330, 0, 4, 0),
	MTK_PIN_INFO(25, 0x7330, 4, 4, 0),
	MTK_PIN_INFO(26, 0x7330, 8, 4, 0),
	MTK_PIN_INFO(27, 0x7330, 12, 4, 0),
	MTK_PIN_INFO(28, 0x7330, 16, 4, 0),
	MTK_PIN_INFO(29, 0x7330, 20, 4, 0),
	MTK_PIN_INFO(30, 0x7330, 24, 4, 0),
	MTK_PIN_INFO(31, 0x7330, 28, 4, 0),
	MTK_PIN_INFO(32, 0x7340, 0, 4, 0),
	MTK_PIN_INFO(33, 0x7340, 4, 4, 0),
	MTK_PIN_INFO(34, 0x7340, 8, 4, 0),
	MTK_PIN_INFO(35, 0x7340, 12, 4, 0),
	MTK_PIN_INFO(36, 0x7340, 16, 4, 0),
	MTK_PIN_INFO(37, 0x7340, 20, 4, 0),
	MTK_PIN_INFO(38, 0x7340, 24, 4, 0),
	MTK_PIN_INFO(39, 0x7340, 28, 4, 0),
	MTK_PIN_INFO(40, 0x7350, 0, 4, 0),
	MTK_PIN_INFO(41, 0x7350, 4, 4, 0),
	MTK_PIN_INFO(42, 0x7350, 8, 4, 0),
	MTK_PIN_INFO(43, 0x7350, 12, 4, 0),
	MTK_PIN_INFO(44, 0x7350, 16, 4, 0),
	MTK_PIN_INFO(45, 0x7350, 20, 4, 0),
	MTK_PIN_INFO(46, 0x7350, 24, 4, 0),
	MTK_PIN_INFO(47, 0x7350, 28, 4, 0),
	MTK_PIN_INFO(48, 0x7360, 0, 4, 0),
	MTK_PIN_INFO(49, 0x7360, 4, 4, 0),
	MTK_PIN_INFO(50, 0x7360, 8, 4, 0),
	MTK_PIN_INFO(51, 0x7360, 12, 4, 0),
	MTK_PIN_INFO(52, 0x7360, 16, 4, 0),
	MTK_PIN_INFO(53, 0x7360, 20, 4, 0),
	MTK_PIN_INFO(54, 0x7360, 24, 4, 0),
	MTK_PIN_INFO(55, 0x7360, 28, 4, 0),
	MTK_PIN_INFO(56, 0x7370, 0, 4, 0),
	MTK_PIN_INFO(57, 0x7370, 4, 4, 0),
	MTK_PIN_INFO(58, 0x7370, 8, 4, 0),
	MTK_PIN_INFO(59, 0x7370, 12, 4, 0),
	MTK_PIN_INFO(60, 0x7370, 16, 4, 0),
	MTK_PIN_INFO(61, 0x7370, 20, 4, 0),
	MTK_PIN_INFO(62, 0x7370, 24, 4, 0),
	MTK_PIN_INFO(63, 0x7370, 28, 4, 0),
	MTK_PIN_INFO(64, 0x7380, 0, 4, 0),
	MTK_PIN_INFO(65, 0x7380, 4, 4, 0),
	MTK_PIN_INFO(66, 0x7380, 8, 4, 0),
	MTK_PIN_INFO(67, 0x7380, 12, 4, 0),
	MTK_PIN_INFO(68, 0x7380, 16, 4, 0),
	MTK_PIN_INFO(69, 0x7380, 20, 4, 0),
	MTK_PIN_INFO(70, 0x7380, 24, 4, 0),
	MTK_PIN_INFO(71, 0x7380, 28, 4, 0),
	MTK_PIN_INFO(72, 0x7390, 0, 4, 0),
	MTK_PIN_INFO(73, 0x7390, 4, 4, 0),
	MTK_PIN_INFO(74, 0x7390, 8, 4, 0),
	MTK_PIN_INFO(75, 0x7390, 12, 4, 0),
	MTK_PIN_INFO(76, 0x7390, 16, 4, 0),
	MTK_PIN_INFO(77, 0x7390, 20, 4, 0),
	MTK_PIN_INFO(78, 0x7390, 24, 4, 0),
};

static const struct mtk_pin_info mtk_pin_info_dir[] = {
	MTK_PIN_INFO(0, 0x7000, 0, 1, 0),
	MTK_PIN_INFO(1, 0x7000, 1, 1, 0),
	MTK_PIN_INFO(2, 0x7000, 2, 1, 0),
	MTK_PIN_INFO(3, 0x7000, 3, 1, 0),
	MTK_PIN_INFO(4, 0x7000, 4, 1, 0),
	MTK_PIN_INFO(5, 0x7000, 5, 1, 0),
	MTK_PIN_INFO(6, 0x7000, 6, 1, 0),
	MTK_PIN_INFO(7, 0x7000, 7, 1, 0),
	MTK_PIN_INFO(8, 0x7000, 8, 1, 0),
	MTK_PIN_INFO(9, 0x7000, 9, 1, 0),
	MTK_PIN_INFO(10, 0x7000, 10, 1, 0),
	MTK_PIN_INFO(11, 0x7000, 11, 1, 0),
	MTK_PIN_INFO(12, 0x7000, 12, 1, 0),
	MTK_PIN_INFO(13, 0x7000, 13, 1, 0),
	MTK_PIN_INFO(14, 0x7000, 14, 1, 0),
	MTK_PIN_INFO(15, 0x7000, 15, 1, 0),
	MTK_PIN_INFO(16, 0x7000, 16, 1, 0),
	MTK_PIN_INFO(17, 0x7000, 17, 1, 0),
	MTK_PIN_INFO(18, 0x7000, 18, 1, 0),
	MTK_PIN_INFO(19, 0x7000, 19, 1, 0),
	MTK_PIN_INFO(20, 0x7000, 20, 1, 0),
	MTK_PIN_INFO(21, 0x7000, 21, 1, 0),
	MTK_PIN_INFO(22, 0x7000, 22, 1, 0),
	MTK_PIN_INFO(23, 0x7000, 23, 1, 0),
	MTK_PIN_INFO(24, 0x7000, 24, 1, 0),
	MTK_PIN_INFO(25, 0x7000, 25, 1, 0),
	MTK_PIN_INFO(26, 0x7000, 26, 1, 0),
	MTK_PIN_INFO(27, 0x7000, 27, 1, 0),
	MTK_PIN_INFO(28, 0x7000, 28, 1, 0),
	MTK_PIN_INFO(29, 0x7000, 29, 1, 0),
	MTK_PIN_INFO(30, 0x7000, 30, 1, 0),
	MTK_PIN_INFO(31, 0x7000, 31, 1, 0),
	MTK_PIN_INFO(32, 0x7010, 0, 1, 0),
	MTK_PIN_INFO(33, 0x7010, 1, 1, 0),
	MTK_PIN_INFO(34, 0x7010, 2, 1, 0),
	MTK_PIN_INFO(35, 0x7010, 3, 1, 0),
	MTK_PIN_INFO(36, 0x7010, 4, 1, 0),
	MTK_PIN_INFO(37, 0x7010, 5, 1, 0),
	MTK_PIN_INFO(38, 0x7010, 6, 1, 0),
	MTK_PIN_INFO(39, 0x7010, 7, 1, 0),
	MTK_PIN_INFO(40, 0x7010, 8, 1, 0),
	MTK_PIN_INFO(41, 0x7010, 9, 1, 0),
	MTK_PIN_INFO(42, 0x7010, 10, 1, 0),
	MTK_PIN_INFO(43, 0x7010, 11, 1, 0),
	MTK_PIN_INFO(44, 0x7010, 12, 1, 0),
	MTK_PIN_INFO(45, 0x7010, 13, 1, 0),
	MTK_PIN_INFO(46, 0x7010, 14, 1, 0),
	MTK_PIN_INFO(47, 0x7010, 15, 1, 0),
	MTK_PIN_INFO(48, 0x7010, 16, 1, 0),
	MTK_PIN_INFO(49, 0x7010, 17, 1, 0),
	MTK_PIN_INFO(50, 0x7010, 18, 1, 0),
	MTK_PIN_INFO(51, 0x7010, 19, 1, 0),
	MTK_PIN_INFO(52, 0x7010, 20, 1, 0),
	MTK_PIN_INFO(53, 0x7010, 21, 1, 0),
	MTK_PIN_INFO(54, 0x7010, 22, 1, 0),
	MTK_PIN_INFO(55, 0x7010, 23, 1, 0),
	MTK_PIN_INFO(56, 0x7010, 24, 1, 0),
	MTK_PIN_INFO(57, 0x7010, 25, 1, 0),
	MTK_PIN_INFO(58, 0x7010, 26, 1, 0),
	MTK_PIN_INFO(59, 0x7010, 27, 1, 0),
	MTK_PIN_INFO(60, 0x7010, 28, 1, 0),
	MTK_PIN_INFO(61, 0x7010, 29, 1, 0),
	MTK_PIN_INFO(62, 0x7010, 30, 1, 0),
	MTK_PIN_INFO(63, 0x7010, 31, 1, 0),
	MTK_PIN_INFO(64, 0x7020, 0, 1, 0),
	MTK_PIN_INFO(65, 0x7020, 1, 1, 0),
	MTK_PIN_INFO(66, 0x7020, 2, 1, 0),
	MTK_PIN_INFO(67, 0x7020, 3, 1, 0),
	MTK_PIN_INFO(68, 0x7020, 4, 1, 0),
	MTK_PIN_INFO(69, 0x7020, 5, 1, 0),
	MTK_PIN_INFO(70, 0x7020, 6, 1, 0),
	MTK_PIN_INFO(71, 0x7020, 7, 1, 0),
	MTK_PIN_INFO(72, 0x7020, 8, 1, 0),
	MTK_PIN_INFO(73, 0x7020, 9, 1, 0),
	MTK_PIN_INFO(74, 0x7020, 10, 1, 0),
	MTK_PIN_INFO(75, 0x7020, 11, 1, 0),
	MTK_PIN_INFO(76, 0x7020, 12, 1, 0),
	MTK_PIN_INFO(77, 0x7020, 13, 1, 0),
	MTK_PIN_INFO(78, 0x7020, 14, 1, 0),
};

static const struct mtk_pin_info mtk_pin_info_datain[] = {
	MTK_PIN_INFO(0, 0x7200, 0, 1, 0),
	MTK_PIN_INFO(1, 0x7200, 1, 1, 0),
	MTK_PIN_INFO(2, 0x7200, 2, 1, 0),
	MTK_PIN_INFO(3, 0x7200, 3, 1, 0),
	MTK_PIN_INFO(4, 0x7200, 4, 1, 0),
	MTK_PIN_INFO(5, 0x7200, 5, 1, 0),
	MTK_PIN_INFO(6, 0x7200, 6, 1, 0),
	MTK_PIN_INFO(7, 0x7200, 7, 1, 0),
	MTK_PIN_INFO(8, 0x7200, 8, 1, 0),
	MTK_PIN_INFO(9, 0x7200, 9, 1, 0),
	MTK_PIN_INFO(10, 0x7200, 10, 1, 0),
	MTK_PIN_INFO(11, 0x7200, 11, 1, 0),
	MTK_PIN_INFO(12, 0x7200, 12, 1, 0),
	MTK_PIN_INFO(13, 0x7200, 13, 1, 0),
	MTK_PIN_INFO(14, 0x7200, 14, 1, 0),
	MTK_PIN_INFO(15, 0x7200, 15, 1, 0),
	MTK_PIN_INFO(16, 0x7200, 16, 1, 0),
	MTK_PIN_INFO(17, 0x7200, 17, 1, 0),
	MTK_PIN_INFO(18, 0x7200, 18, 1, 0),
	MTK_PIN_INFO(19, 0x7200, 19, 1, 0),
	MTK_PIN_INFO(20, 0x7200, 20, 1, 0),
	MTK_PIN_INFO(21, 0x7200, 21, 1, 0),
	MTK_PIN_INFO(22, 0x7200, 22, 1, 0),
	MTK_PIN_INFO(23, 0x7200, 23, 1, 0),
	MTK_PIN_INFO(24, 0x7200, 24, 1, 0),
	MTK_PIN_INFO(25, 0x7200, 25, 1, 0),
	MTK_PIN_INFO(26, 0x7200, 26, 1, 0),
	MTK_PIN_INFO(27, 0x7200, 27, 1, 0),
	MTK_PIN_INFO(28, 0x7200, 28, 1, 0),
	MTK_PIN_INFO(29, 0x7200, 29, 1, 0),
	MTK_PIN_INFO(30, 0x7200, 30, 1, 0),
	MTK_PIN_INFO(31, 0x7200, 31, 1, 0),
	MTK_PIN_INFO(32, 0x7210, 0, 1, 0),
	MTK_PIN_INFO(33, 0x7210, 1, 1, 0),
	MTK_PIN_INFO(34, 0x7210, 2, 1, 0),
	MTK_PIN_INFO(35, 0x7210, 3, 1, 0),
	MTK_PIN_INFO(36, 0x7210, 4, 1, 0),
	MTK_PIN_INFO(37, 0x7210, 5, 1, 0),
	MTK_PIN_INFO(38, 0x7210, 6, 1, 0),
	MTK_PIN_INFO(39, 0x7210, 7, 1, 0),
	MTK_PIN_INFO(40, 0x7210, 8, 1, 0),
	MTK_PIN_INFO(41, 0x7210, 9, 1, 0),
	MTK_PIN_INFO(42, 0x7210, 10, 1, 0),
	MTK_PIN_INFO(43, 0x7210, 11, 1, 0),
	MTK_PIN_INFO(44, 0x7210, 12, 1, 0),
	MTK_PIN_INFO(45, 0x7210, 13, 1, 0),
	MTK_PIN_INFO(46, 0x7210, 14, 1, 0),
	MTK_PIN_INFO(47, 0x7210, 15, 1, 0),
	MTK_PIN_INFO(48, 0x7210, 16, 1, 0),
	MTK_PIN_INFO(49, 0x7210, 17, 1, 0),
	MTK_PIN_INFO(50, 0x7210, 18, 1, 0),
	MTK_PIN_INFO(51, 0x7210, 19, 1, 0),
	MTK_PIN_INFO(52, 0x7210, 20, 1, 0),
	MTK_PIN_INFO(53, 0x7210, 21, 1, 0),
	MTK_PIN_INFO(54, 0x7210, 22, 1, 0),
	MTK_PIN_INFO(55, 0x7210, 23, 1, 0),
	MTK_PIN_INFO(56, 0x7210, 24, 1, 0),
	MTK_PIN_INFO(57, 0x7210, 25, 1, 0),
	MTK_PIN_INFO(58, 0x7210, 26, 1, 0),
	MTK_PIN_INFO(59, 0x7210, 27, 1, 0),
	MTK_PIN_INFO(60, 0x7210, 28, 1, 0),
	MTK_PIN_INFO(61, 0x7210, 29, 1, 0),
	MTK_PIN_INFO(62, 0x7210, 30, 1, 0),
	MTK_PIN_INFO(63, 0x7210, 31, 1, 0),
	MTK_PIN_INFO(64, 0x7220, 0, 1, 0),
	MTK_PIN_INFO(65, 0x7220, 1, 1, 0),
	MTK_PIN_INFO(66, 0x7220, 2, 1, 0),
	MTK_PIN_INFO(67, 0x7220, 3, 1, 0),
	MTK_PIN_INFO(68, 0x7220, 4, 1, 0),
	MTK_PIN_INFO(69, 0x7220, 5, 1, 0),
	MTK_PIN_INFO(70, 0x7220, 6, 1, 0),
	MTK_PIN_INFO(71, 0x7220, 7, 1, 0),
	MTK_PIN_INFO(72, 0x7220, 8, 1, 0),
	MTK_PIN_INFO(73, 0x7220, 9, 1, 0),
	MTK_PIN_INFO(74, 0x7220, 10, 1, 0),
	MTK_PIN_INFO(75, 0x7220, 11, 1, 0),
	MTK_PIN_INFO(76, 0x7220, 12, 1, 0),
	MTK_PIN_INFO(77, 0x7220, 13, 1, 0),
	MTK_PIN_INFO(78, 0x7220, 14, 1, 0),
};

static const struct mtk_pin_info mtk_pin_info_dataout[] = {
	MTK_PIN_INFO(0, 0x7100, 0, 1, 0),
	MTK_PIN_INFO(1, 0x7100, 1, 1, 0),
	MTK_PIN_INFO(2, 0x7100, 2, 1, 0),
	MTK_PIN_INFO(3, 0x7100, 3, 1, 0),
	MTK_PIN_INFO(4, 0x7100, 4, 1, 0),
	MTK_PIN_INFO(5, 0x7100, 5, 1, 0),
	MTK_PIN_INFO(6, 0x7100, 6, 1, 0),
	MTK_PIN_INFO(7, 0x7100, 7, 1, 0),
	MTK_PIN_INFO(8, 0x7100, 8, 1, 0),
	MTK_PIN_INFO(9, 0x7100, 9, 1, 0),
	MTK_PIN_INFO(10, 0x7100, 10, 1, 0),
	MTK_PIN_INFO(11, 0x7100, 11, 1, 0),
	MTK_PIN_INFO(12, 0x7100, 12, 1, 0),
	MTK_PIN_INFO(13, 0x7100, 13, 1, 0),
	MTK_PIN_INFO(14, 0x7100, 14, 1, 0),
	MTK_PIN_INFO(15, 0x7100, 15, 1, 0),
	MTK_PIN_INFO(16, 0x7100, 16, 1, 0),
	MTK_PIN_INFO(17, 0x7100, 17, 1, 0),
	MTK_PIN_INFO(18, 0x7100, 18, 1, 0),
	MTK_PIN_INFO(19, 0x7100, 19, 1, 0),
	MTK_PIN_INFO(20, 0x7100, 20, 1, 0),
	MTK_PIN_INFO(21, 0x7100, 21, 1, 0),
	MTK_PIN_INFO(22, 0x7100, 22, 1, 0),
	MTK_PIN_INFO(23, 0x7100, 23, 1, 0),
	MTK_PIN_INFO(24, 0x7100, 24, 1, 0),
	MTK_PIN_INFO(25, 0x7100, 25, 1, 0),
	MTK_PIN_INFO(26, 0x7100, 26, 1, 0),
	MTK_PIN_INFO(27, 0x7100, 27, 1, 0),
	MTK_PIN_INFO(28, 0x7100, 28, 1, 0),
	MTK_PIN_INFO(29, 0x7100, 29, 1, 0),
	MTK_PIN_INFO(30, 0x7100, 30, 1, 0),
	MTK_PIN_INFO(31, 0x7100, 31, 1, 0),
	MTK_PIN_INFO(32, 0x7110, 0, 1, 0),
	MTK_PIN_INFO(33, 0x7110, 1, 1, 0),
	MTK_PIN_INFO(34, 0x7110, 2, 1, 0),
	MTK_PIN_INFO(35, 0x7110, 3, 1, 0),
	MTK_PIN_INFO(36, 0x7110, 4, 1, 0),
	MTK_PIN_INFO(37, 0x7110, 5, 1, 0),
	MTK_PIN_INFO(38, 0x7110, 6, 1, 0),
	MTK_PIN_INFO(39, 0x7110, 7, 1, 0),
	MTK_PIN_INFO(40, 0x7110, 8, 1, 0),
	MTK_PIN_INFO(41, 0x7110, 9, 1, 0),
	MTK_PIN_INFO(42, 0x7110, 10, 1, 0),
	MTK_PIN_INFO(43, 0x7110, 11, 1, 0),
	MTK_PIN_INFO(44, 0x7110, 12, 1, 0),
	MTK_PIN_INFO(45, 0x7110, 13, 1, 0),
	MTK_PIN_INFO(46, 0x7110, 14, 1, 0),
	MTK_PIN_INFO(47, 0x7110, 15, 1, 0),
	MTK_PIN_INFO(48, 0x7110, 16, 1, 0),
	MTK_PIN_INFO(49, 0x7110, 17, 1, 0),
	MTK_PIN_INFO(50, 0x7110, 18, 1, 0),
	MTK_PIN_INFO(51, 0x7110, 19, 1, 0),
	MTK_PIN_INFO(52, 0x7110, 20, 1, 0),
	MTK_PIN_INFO(53, 0x7110, 21, 1, 0),
	MTK_PIN_INFO(54, 0x7110, 22, 1, 0),
	MTK_PIN_INFO(55, 0x7110, 23, 1, 0),
	MTK_PIN_INFO(56, 0x7110, 24, 1, 0),
	MTK_PIN_INFO(57, 0x7110, 25, 1, 0),
	MTK_PIN_INFO(58, 0x7110, 26, 1, 0),
	MTK_PIN_INFO(59, 0x7110, 27, 1, 0),
	MTK_PIN_INFO(60, 0x7110, 28, 1, 0),
	MTK_PIN_INFO(61, 0x7110, 29, 1, 0),
	MTK_PIN_INFO(62, 0x7110, 30, 1, 0),
	MTK_PIN_INFO(63, 0x7110, 31, 1, 0),
	MTK_PIN_INFO(64, 0x7120, 0, 1, 0),
	MTK_PIN_INFO(65, 0x7120, 1, 1, 0),
	MTK_PIN_INFO(66, 0x7120, 2, 1, 0),
	MTK_PIN_INFO(67, 0x7120, 3, 1, 0),
	MTK_PIN_INFO(68, 0x7120, 4, 1, 0),
	MTK_PIN_INFO(69, 0x7120, 5, 1, 0),
	MTK_PIN_INFO(70, 0x7120, 6, 1, 0),
	MTK_PIN_INFO(71, 0x7120, 7, 1, 0),
	MTK_PIN_INFO(72, 0x7120, 8, 1, 0),
	MTK_PIN_INFO(73, 0x7120, 9, 1, 0),
	MTK_PIN_INFO(74, 0x7120, 10, 1, 0),
	MTK_PIN_INFO(75, 0x7120, 11, 1, 0),
	MTK_PIN_INFO(76, 0x7120, 12, 1, 0),
	MTK_PIN_INFO(77, 0x7120, 13, 1, 0),
	MTK_PIN_INFO(78, 0x7120, 14, 1, 0),
};

static const struct mtk_pin_info mtk_pin_info_pullen[] = {
	MTK_PIN_INFO(0, 0x8400, 0, 1, 0),
	MTK_PIN_INFO(1, 0x8400, 1, 1, 0),
	MTK_PIN_INFO(2, 0x8400, 2, 1, 0),
	MTK_PIN_INFO(3, 0x8400, 3, 1, 0),
	MTK_PIN_INFO(4, 0x8400, 4, 1, 0),
	MTK_PIN_INFO(5, 0x8400, 5, 1, 0),
	MTK_PIN_INFO(6, 0x8400, 6, 1, 0),
	MTK_PIN_INFO(7, 0x8400, 7, 1, 0),
	MTK_PIN_INFO(8, 0x8400, 8, 1, 0),
	MTK_PIN_INFO(9, 0x8400, 9, 1, 0),
	MTK_PIN_INFO(10, 0x8400, 10, 1, 0),
	MTK_PIN_INFO(11, 0x9400, 0, 1, 0),
	MTK_PIN_INFO(12, 0x9400, 1, 1, 0),
	MTK_PIN_INFO(13, 0x9400, 2, 1, 0),
	MTK_PIN_INFO(14, 0x9400, 3, 1, 0),
	MTK_PIN_INFO(15, 0x9400, 4, 1, 0),
	MTK_PIN_INFO(16, 0x9400, 5, 1, 0),
	MTK_PIN_INFO(17, 0x9400, 6, 1, 0),
	MTK_PIN_INFO(18, 0x9400, 7, 1, 0),
	MTK_PIN_INFO(19, 0xa400, 0, 1, 0),
	MTK_PIN_INFO(20, 0xa400, 1, 1, 0),
	MTK_PIN_INFO(21, 0xa400, 2, 1, 0),
	MTK_PIN_INFO(22, 0xa400, 3, 1, 0),
	MTK_PIN_INFO(23, 0xa400, 4, 1, 0),
	MTK_PIN_INFO(24, 0xa400, 5, 1, 0),
	MTK_PIN_INFO(25, 0xa400, 6, 1, 0),
	MTK_PIN_INFO(26, 0xa400, 7, 1, 0),
	MTK_PIN_INFO(27, 0xa400, 8, 1, 0),
	MTK_PIN_INFO(28, 0xa400, 9, 1, 0),
	MTK_PIN_INFO(29, 0xa400, 10, 1, 0),
	MTK_PIN_INFO(30, 0xa400, 11, 1, 0),
	MTK_PIN_INFO(31, 0xa400, 12, 1, 0),
	MTK_PIN_INFO(32, 0xa400, 13, 1, 0),
	MTK_PIN_INFO(33, 0xb400, 0, 1, 0),
	MTK_PIN_INFO(34, 0xb400, 1, 1, 0),
	MTK_PIN_INFO(35, 0xb400, 2, 1, 0),
	MTK_PIN_INFO(36, 0xb400, 3, 1, 0),
	MTK_PIN_INFO(37, 0xb400, 4, 1, 0),
	MTK_PIN_INFO(38, 0xb400, 5, 1, 0),
	MTK_PIN_INFO(39, 0xb400, 6, 1, 0),
	MTK_PIN_INFO(40, 0xb400, 7, 1, 0),
	MTK_PIN_INFO(41, 0xb400, 8, 1, 0),
	MTK_PIN_INFO(42, 0xb400, 9, 1, 0),
	MTK_PIN_INFO(43, 0xb400, 10, 1, 0),
	MTK_PIN_INFO(44, 0xb400, 11, 1, 0),
	MTK_PIN_INFO(45, 0xb400, 12, 1, 0),
	MTK_PIN_INFO(46, 0xb400, 13, 1, 0),
	MTK_PIN_INFO(47, 0xb400, 14, 1, 0),
	MTK_PIN_INFO(48, 0xb400, 15, 1, 0),
	MTK_PIN_INFO(49, 0xc400, 0, 1, 0),
	MTK_PIN_INFO(50, 0xc400, 1, 1, 0),
	MTK_PIN_INFO(51, 0xd400, 0, 1, 0),
	MTK_PIN_INFO(52, 0xd400, 1, 1, 0),
	MTK_PIN_INFO(53, 0xd400, 2, 1, 0),
	MTK_PIN_INFO(54, 0xd400, 3, 1, 0),
	MTK_PIN_INFO(55, 0xd400, 4, 1, 0),
	MTK_PIN_INFO(56, 0xd400, 5, 1, 0),
	MTK_PIN_INFO(57, 0xd400, 6, 1, 0),
	MTK_PIN_INFO(58, 0xd400, 7, 1, 0),
	MTK_PIN_INFO(59, 0xd400, 8, 1, 0),
	MTK_PIN_INFO(60, 0xd400, 9, 1, 0),
	MTK_PIN_INFO(61, 0xd400, 10, 1, 0),
	MTK_PIN_INFO(62, 0xd400, 11, 1, 0),
	MTK_PIN_INFO(63, 0xd400, 12, 1, 0),
	MTK_PIN_INFO(64, 0xd400, 13, 1, 0),
	MTK_PIN_INFO(65, 0xd400, 14, 1, 0),
	MTK_PIN_INFO(66, 0xd400, 15, 1, 0),
	MTK_PIN_INFO(67, 0xd400, 16, 1, 0),
	MTK_PIN_INFO(68, 0xd400, 17, 1, 0),
	MTK_PIN_INFO(69, 0xd400, 18, 1, 0),
	MTK_PIN_INFO(70, 0xe400, 0, 1, 0),
	MTK_PIN_INFO(71, 0xe400, 1, 1, 0),
	MTK_PIN_INFO(72, 0xe400, 2, 1, 0),
	MTK_PIN_INFO(73, 0xe400, 3, 1, 0),
	MTK_PIN_INFO(74, 0xe400, 4, 1, 0),
	MTK_PIN_INFO(75, 0xe400, 5, 1, 0),
	MTK_PIN_INFO(76, 0xe400, 6, 1, 0),
	MTK_PIN_INFO(77, 0xe400, 7, 1, 0),
	MTK_PIN_INFO(78, 0xe400, 8, 1, 0),
};

static const struct mtk_pin_info mtk_pin_info_pullsel[] = {
	MTK_PIN_INFO(0, 0x8500, 0, 1, 0),
	MTK_PIN_INFO(1, 0x8500, 1, 1, 0),
	MTK_PIN_INFO(2, 0x8500, 2, 1, 0),
	MTK_PIN_INFO(3, 0x8500, 3, 1, 0),
	MTK_PIN_INFO(4, 0x8500, 4, 1, 0),
	MTK_PIN_INFO(5, 0x8500, 5, 1, 0),
	MTK_PIN_INFO(6, 0x8500, 6, 1, 0),
	MTK_PIN_INFO(7, 0x8500, 7, 1, 0),
	MTK_PIN_INFO(8, 0x8500, 8, 1, 0),
	MTK_PIN_INFO(9, 0x8500, 9, 1, 0),
	MTK_PIN_INFO(10, 0x8500, 10, 1, 0),
	MTK_PIN_INFO(11, 0x9500, 0, 1, 0),
	MTK_PIN_INFO(12, 0x9500, 1, 1, 0),
	MTK_PIN_INFO(13, 0x9500, 2, 1, 0),
	MTK_PIN_INFO(14, 0x9500, 3, 1, 0),
	MTK_PIN_INFO(15, 0x9500, 4, 1, 0),
	MTK_PIN_INFO(16, 0x9500, 5, 1, 0),
	MTK_PIN_INFO(17, 0x9500, 6, 1, 0),
	MTK_PIN_INFO(18, 0x9500, 7, 1, 0),
	MTK_PIN_INFO(19, 0xa500, 0, 1, 0),
	MTK_PIN_INFO(20, 0xa500, 1, 1, 0),
	MTK_PIN_INFO(21, 0xa500, 2, 1, 0),
	MTK_PIN_INFO(22, 0xa500, 3, 1, 0),
	MTK_PIN_INFO(23, 0xa500, 4, 1, 0),
	MTK_PIN_INFO(24, 0xa500, 5, 1, 0),
	MTK_PIN_INFO(25, 0xa500, 6, 1, 0),
	MTK_PIN_INFO(26, 0xa500, 7, 1, 0),
	MTK_PIN_INFO(27, 0xa500, 8, 1, 0),
	MTK_PIN_INFO(28, 0xa500, 9, 1, 0),
	MTK_PIN_INFO(29, 0xa500, 10, 1, 0),
	MTK_PIN_INFO(30, 0xa500, 11, 1, 0),
	MTK_PIN_INFO(31, 0xa500, 12, 1, 0),
	MTK_PIN_INFO(32, 0xa500, 13, 1, 0),
	MTK_PIN_INFO(33, 0xb500, 0, 1, 0),
	MTK_PIN_INFO(34, 0xb500, 1, 1, 0),
	MTK_PIN_INFO(35, 0xb500, 2, 1, 0),
	MTK_PIN_INFO(36, 0xb500, 3, 1, 0),
	MTK_PIN_INFO(37, 0xb500, 4, 1, 0),
	MTK_PIN_INFO(38, 0xb500, 5, 1, 0),
	MTK_PIN_INFO(39, 0xb500, 6, 1, 0),
	MTK_PIN_INFO(40, 0xb500, 7, 1, 0),
	MTK_PIN_INFO(41, 0xb500, 8, 1, 0),
	MTK_PIN_INFO(42, 0xb500, 9, 1, 0),
	MTK_PIN_INFO(43, 0xb500, 10, 1, 0),
	MTK_PIN_INFO(44, 0xb500, 11, 1, 0),
	MTK_PIN_INFO(45, 0xb500, 12, 1, 0),
	MTK_PIN_INFO(46, 0xb500, 13, 1, 0),
	MTK_PIN_INFO(47, 0xb500, 14, 1, 0),
	MTK_PIN_INFO(48, 0xb500, 15, 1, 0),
	MTK_PIN_INFO(49, 0xc500, 0, 1, 0),
	MTK_PIN_INFO(50, 0xc500, 1, 1, 0),
	MTK_PIN_INFO(51, 0xd500, 0, 1, 0),
	MTK_PIN_INFO(52, 0xd500, 1, 1, 0),
	MTK_PIN_INFO(53, 0xd500, 2, 1, 0),
	MTK_PIN_INFO(54, 0xd500, 3, 1, 0),
	MTK_PIN_INFO(55, 0xd500, 4, 1, 0),
	MTK_PIN_INFO(56, 0xd500, 5, 1, 0),
	MTK_PIN_INFO(57, 0xd500, 6, 1, 0),
	MTK_PIN_INFO(58, 0xd500, 7, 1, 0),
	MTK_PIN_INFO(59, 0xd500, 8, 1, 0),
	MTK_PIN_INFO(60, 0xd500, 9, 1, 0),
	MTK_PIN_INFO(61, 0xd500, 10, 1, 0),
	MTK_PIN_INFO(62, 0xd500, 11, 1, 0),
	MTK_PIN_INFO(63, 0xd500, 12, 1, 0),
	MTK_PIN_INFO(64, 0xd500, 13, 1, 0),
	MTK_PIN_INFO(65, 0xd500, 14, 1, 0),
	MTK_PIN_INFO(66, 0xd500, 15, 1, 0),
	MTK_PIN_INFO(67, 0xd500, 16, 1, 0),
	MTK_PIN_INFO(68, 0xd500, 17, 1, 0),
	MTK_PIN_INFO(69, 0xd500, 18, 1, 0),
	MTK_PIN_INFO(70, 0xe500, 0, 1, 0),
	MTK_PIN_INFO(71, 0xe500, 1, 1, 0),
	MTK_PIN_INFO(72, 0xe500, 2, 1, 0),
	MTK_PIN_INFO(73, 0xe500, 3, 1, 0),
	MTK_PIN_INFO(74, 0xe500, 4, 1, 0),
	MTK_PIN_INFO(75, 0xe500, 5, 1, 0),
	MTK_PIN_INFO(76, 0xe500, 6, 1, 0),
	MTK_PIN_INFO(77, 0xe500, 7, 1, 0),
	MTK_PIN_INFO(78, 0xe500, 8, 1, 0),
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

	if (!select) {
		mtk_set_pin_value(pin, 0, MAX_GPIO_PIN, mtk_pin_info_pullen);
	} else {
		mtk_set_pin_value(pin, 1, MAX_GPIO_PIN, mtk_pin_info_pullen);
		mtk_set_pin_value(pin, select == GPIO_PULL_UP,
				  MAX_GPIO_PIN, mtk_pin_info_pullsel);
	}
}

static int mtk_get_pin_pull_select_chip(uint32_t pin)
{
	uint32_t pull_sel, pull_en;

	pull_en = mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_pullen);
	pull_sel = mtk_get_pin_value(pin, MAX_GPIO_PIN, mtk_pin_info_pullsel);

	if (!pull_en)
		return GPIO_PULL_NONE;

	return pull_sel ? GPIO_PULL_UP : GPIO_PULL_DOWN;
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
