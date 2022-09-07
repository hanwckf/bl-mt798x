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
	CB_CKSQ_40M = (40 * 1000 * 1000), //40Mhz
	CB_MPLL_D2 = (208 * 1000 * 1000), //208M
	CB_MMPLL_D4	= (180 * 1000 * 1000), //180M
	NET1PLL_D8_D2 = (15625 * 10 * 1000), //156.25M
	CB_NET2PLL_D6 = (133333 * 1 * 1000), //133.333M
	NET1PLL_D5_D4 = (125 * 1000 * 1000), //125M
	CB_MPLL_D4 = (104 * 1000 * 1000), //104M
	NET1PLL_D8_D4 = (78125 * 1 * 1000), //78.125M
} spi_pll;

typedef enum {
	SPIM0 = 0,
	SPIM2 = 2,
}spim_port;


void mtk_spi_source_clock_select(spi_pll src_clk);
void mtk_spi_gpio_init(spim_port port);

#endif /* MT_GPIO_H */
