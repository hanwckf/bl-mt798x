/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <boot_spi.h>

#define MTK_QSPI_SRC_CLK		CLK_MPLL_D2

uint32_t mtk_plat_get_qspi_src_clk(void)
{
	/* config GPIO pinmux to spi mode */
	mtk_spi_gpio_init();

	/* select 208M clk */
	mtk_spi_source_clock_select(MTK_QSPI_SRC_CLK);

	return MTK_QSPI_SRC_CLK;
}
