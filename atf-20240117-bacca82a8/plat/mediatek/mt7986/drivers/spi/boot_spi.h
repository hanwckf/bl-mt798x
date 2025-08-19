/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_SPI_H
#define BOOT_SPI_H

#include <stdbool.h>
#include <stdint.h>
#include <plat/common/common_def.h>

typedef enum {
	CLK_XTAL_DEF = (40 * 1000 * 1000), //real IC default clk is 40Mhz
	CLK_MPLL_D2 = (208 * 1000 * 1000),
	CLK_MPLL_D8 = (1707 * 100 * 1000),
	CLK_PLL_D8_D2 = (1562 * 100 * 1000),
	CLK_PLL_D3_D2 = (1333 * 100 * 1000),
	CLK_PLL_D5_D4 = (125 * 1000 * 1000),
	CLK_MPLL_D4 = (104 * 1000 * 1000),
	CLK_PLL_D5_D2 = (76 * 1000 * 1000),
} spi_pll;

void mtk_spi_source_clock_select(spi_pll src_clk);
void mtk_spi_gpio_init(void);

#endif /* MT_GPIO_H */
