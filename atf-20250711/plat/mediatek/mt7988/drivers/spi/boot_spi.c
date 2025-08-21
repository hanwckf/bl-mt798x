/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <assert.h>
#include <common/debug.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <mt7988_gpio.h> /* need to migrate to mt7988 later */
#include <mt7988_def.h>
#include <boot_spi.h>

void mtk_spi_gpio_init(spim_port port)
{
	if (port == SPIM0) {
		/*spi0:gpio22-gpio27,mode:1*/
		mt_set_pinmux_mode(GPIO22, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO23, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO24, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO25, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO26, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO27, GPIO_MODE_01);

		mmio_clrsetbits_32(GPIO_RB_PUPD_CFG0,
			0b111111 << SPI0_PUPD_S, 0b011001 << SPI0_PUPD_S);

		/* driving */
		mmio_clrsetbits_32(GPIO_RB_DRV_CFG1,
			0x7 << SPI0_CS_DRV_S   | 0x7 << SPI0_CLK_DRV_S,
			0x1 << SPI0_CS_DRV_S   | 0x1 << SPI0_CLK_DRV_S);

		mmio_clrsetbits_32(GPIO_RB_DRV_CFG2,
			0x7 << SPI0_WP_DRV_S   | 0x7 << SPI0_MOSI_DRV_S |
			0x7 << SPI0_MISO_DRV_S | 0x7 << SPI0_HOLD_DRV_S,
			0x1 << SPI0_WP_DRV_S   | 0x1 << SPI0_MOSI_DRV_S |
			0x1 << SPI0_MISO_DRV_S | 0x1 << SPI0_HOLD_DRV_S);
	} else if (port == SPIM2) {
		/*spi2:gpio32-gpio37,mode:1*/
		mt_set_pinmux_mode(GPIO32, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO33, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO34, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO35, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO36, GPIO_MODE_01);
		mt_set_pinmux_mode(GPIO37, GPIO_MODE_01);

		mmio_clrsetbits_32(GPIO_RB_PUPD_CFG0,
			(unsigned)0b1111 << SPI2_PUPD_S1, (unsigned)0b1001 << SPI2_PUPD_S1);
		mmio_clrsetbits_32(GPIO_RB_PUPD_CFG1,
			0b11 << SPI2_PUPD_S2, 0b01 << SPI2_PUPD_S2);

		/* driving */
		mmio_clrsetbits_32(GPIO_RB_DRV_CFG2,
			0x7 << SPI2_CS_DRV_S   | 0x7 << SPI2_CLK_DRV_S,
			0x3 << SPI2_CS_DRV_S   | 0x3 << SPI2_CLK_DRV_S);

		mmio_clrsetbits_32(GPIO_RB_DRV_CFG3,
			0x7 << SPI2_WP_DRV_S   | 0x7 << SPI2_MOSI_DRV_S |
			0x7 << SPI2_MISO_DRV_S | 0x7 << SPI2_HOLD_DRV_S,
			0x3 << SPI2_WP_DRV_S   | 0x3 << SPI2_MOSI_DRV_S |
			0x3 << SPI2_MISO_DRV_S | 0x3 << SPI2_HOLD_DRV_S);
	}
}
void mtk_spi_source_clock_select(spi_pll src_clk)
{
	int reg_val = 0;
	switch (src_clk) {
	case CB_CKSQ_40M:
		reg_val |= CLK_SPIM_40MHz;
		break;
	case CB_MPLL_D2:
		reg_val |= CLK_SPIM_208MHz;
		break;
	case CB_MMPLL_D4:
		reg_val |= CLK_SPIM_180MHz;
		break;
	case NET1PLL_D8_D2:
		reg_val |= CLK_SPIM_156MHz;
		break;
	case CB_NET2PLL_D6:
		reg_val |= CLK_SPIM_133MHz;
		break;
	case NET1PLL_D5_D4:
		reg_val |= CLK_SPIM_125MHz;
		break;
	case CB_MPLL_D4:
		reg_val |= CLK_SPIM_104MHz;
		break;
	case NET1PLL_D8_D4:
		reg_val |= CLK_SPIM_76MHz;
		break;
	default:
		reg_val |= CLK_SPIM_40MHz;
		break;
	}
	mmio_write_32(CLK_CFG_3_CLR, CLK_SPI_SEL_MASK);
	mmio_write_32(CLK_CFG_3_SET, reg_val << CLK_SPI_SEL_S);
	mmio_write_32(CLK_CFG_UPDATE, SPI_CK_UPDATE);
}
