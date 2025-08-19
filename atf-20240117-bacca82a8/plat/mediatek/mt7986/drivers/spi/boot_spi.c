/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <mt7986_gpio.h>
#include <boot_spi.h>

void mtk_spi_gpio_init(void)
{
	/* Pinmux */
	mmio_clrsetbits_32(GPIO_MODE4,
		0x7 << GPIO_PIN33_S | 0x7 << GPIO_PIN34_S | 0x7 << GPIO_PIN35_S |
		0x7 << GPIO_PIN36_S | 0x7 << GPIO_PIN37_S | 0x7 << GPIO_PIN38_S,
		0x1 << GPIO_PIN33_S | 0x1 << GPIO_PIN34_S | 0x1 << GPIO_PIN35_S |
		0x1 << GPIO_PIN36_S | 0x1 << GPIO_PIN37_S | 0x1 << GPIO_PIN38_S);

	/* PU/PD */
	mmio_clrsetbits_32(GPIO_LT_PUPD_CFG0, 0b111111 << SPI2_PUPD_S, 0b011001 << SPI2_PUPD_S);

	/* driving */
	mmio_clrsetbits_32(GPIO_LT_DRV_CFG0,
		0x7 << SPI2_WP_DRV_S   | 0x7 << SPI2_MOSI_DRV_S | 0x7 << SPI2_MISO_DRV_S |
		0x7 << SPI2_HOLD_DRV_S | 0x7 << SPI2_CS_DRV_S   | 0x7 << SPI2_CLK_DRV_S,
		0x3 << SPI2_WP_DRV_S   | 0x3 << SPI2_MOSI_DRV_S | 0x3 << SPI2_MISO_DRV_S |
		0x3 << SPI2_HOLD_DRV_S | 0x3 << SPI2_CS_DRV_S   | 0x3 << SPI2_CLK_DRV_S);
}

void mtk_spi_source_clock_select(spi_pll src_clk)
{
	int reg_val = 0;

	switch (src_clk) {
		case CLK_XTAL_DEF:
			reg_val |= CLK_SPIM_40MHz;
			break;
		case CLK_MPLL_D2:
			reg_val |= CLK_SPIM_208MHz;
			break;
		case CLK_MPLL_D8:
			reg_val |= CLK_SPIM_180MHz;
			break;
		case CLK_PLL_D8_D2:
			reg_val |= CLK_SPIM_156MHz;
			break;
		case CLK_PLL_D3_D2:
			reg_val |= CLK_SPIM_133MHz;
			break;
		case CLK_PLL_D5_D4:
			reg_val |= CLK_SPIM_125MHz;
			break;
		case CLK_MPLL_D4:
			reg_val |= CLK_SPIM_104MHz;
			break;
		case CLK_PLL_D5_D2:
			reg_val |= CLK_SPIM_76MHz;
			break;
		default:
			reg_val |= CLK_SPIM_40MHz;
			break;
	}

	mmio_write_32(CLK_CFG_0_CLR, CLK_SPI_SEL_MASK);
	mmio_write_32(CLK_CFG_0_SET, reg_val << CLK_SPI_SEL_S);
	mmio_write_32(CLK_CFG_UPDATE, SPI_CK_UPDATE);
}
