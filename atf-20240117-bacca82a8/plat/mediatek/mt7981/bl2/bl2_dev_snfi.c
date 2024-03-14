/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <drivers/delay_timer.h>
#include <platform_def.h>
#include <mt7981_gpio.h>
#include <mtk-snand.h>

#define FIP_BASE			0x380000
#define FIP_SIZE			0x200000

static const struct mtk_snand_platdata mt7981_snand_pdata = {
	.nfi_base = (void *)NFI_BASE,
	.ecc_base = (void *)NFI_ECC_BASE,
	.soc = SNAND_SOC_MT7981,
	.quad_spi = true
};

static void snand_gpio_clk_setup(void)
{
	/* Reset */
	mmio_setbits_32(0x10001080, 1 << 2);
	udelay(1000);
	mmio_setbits_32(0x10001084, 1 << 2);

	/* TOPCKGEN CFG0 nfi1x */
	mmio_write_32(CLK_CFG_0_CLR, CLK_NFI1X_SEL_MASK);
	mmio_write_32(CLK_CFG_0_SET, CLK_NFI1X_52MHz << CLK_NFI1X_SEL_S);

	/* TOPCKGEN CFG0 spinfi */
	mmio_write_32(CLK_CFG_0_CLR, CLK_SPINFI_BCLK_SEL_MASK);
	mmio_write_32(CLK_CFG_0_SET, CLK_NFI1X_52MHz << CLK_SPINFI_BCLK_SEL_S);

	mmio_write_32(CLK_CFG_UPDATE, NFI1X_CK_UPDATE | SPINFI_CK_UPDATE);

	/* GPIO mode */
	mmio_clrsetbits_32(GPIO_MODE2, (0x7 << GPIO_PIN21_S) |
			   (0x7 << GPIO_PIN20_S) | (0x7 << GPIO_PIN19_S) |
			   (0x7 << GPIO_PIN18_S) | (0x7 << GPIO_PIN17_S) |
			   (0x7 << GPIO_PIN16_S),
			   (0x3 << GPIO_PIN21_S) | (0x3 << GPIO_PIN20_S) |
			   (0x3 << GPIO_PIN19_S) | (0x3 << GPIO_PIN18_S) |
			   (0x3 << GPIO_PIN17_S) | (0x3 << GPIO_PIN16_S));

	/* GPIO PUPD */
	mmio_clrsetbits_32(GPIO_RM_PUPD_CFG0, 0b111111 << SPI0_PUPD_S,
			   0b011001 << SPI0_PUPD_S);
	mmio_clrsetbits_32(GPIO_RM_R0_CFG0, 0b111111 << SPI0_R0_S,
			   0b100110 << SPI0_R0_S);
	mmio_clrsetbits_32(GPIO_RM_R1_CFG0, 0b111111 << SPI0_R1_S,
			   0b011001 << SPI0_R1_S);

	/* GPIO driving */
	mmio_clrsetbits_32(GPIO_RM_DRV_CFG0, (0x7 << SPI0_WP_DRV_S) |
			   (0x7 << SPI0_MOSI_DRV_S) | (0x7 << SPI0_MISO_DRV_S) |
			   (0x7 << SPI0_HOLD_DRV_S) | (0x7 << SPI0_CS_DRV_S) |
			   (0x7 << SPI0_CLK_DRV_S),
			   (0x2 << SPI0_WP_DRV_S) | (0x2 << SPI0_MOSI_DRV_S) |
			   (0x2 << SPI0_MISO_DRV_S) | (0x2 << SPI0_HOLD_DRV_S) |
			   (0x2 << SPI0_CS_DRV_S) | (0x3 << SPI0_CLK_DRV_S));
}

const struct mtk_snand_platdata *mtk_plat_get_snfi_platdata(void)
{
	snand_gpio_clk_setup();

	return &mt7981_snand_pdata;
}

void mtk_plat_fip_location(size_t *fip_off, size_t *fip_size)
{
	*fip_off = FIP_BASE;
	*fip_size = FIP_SIZE;
}
