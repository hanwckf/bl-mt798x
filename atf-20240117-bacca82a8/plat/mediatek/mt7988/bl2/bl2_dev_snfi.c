/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <drivers/delay_timer.h>
#include <mt7988_gpio.h>
#include <mt7988_def.h>
#include <mtk-snand.h>

#define FIP_BASE			0x580000
#define FIP_SIZE			0x200000

static const struct mtk_snand_platdata mt7988_snand_pdata = {
	.nfi_base = (void *)NFI_BASE,
	.ecc_base = (void *)NFI_ECC_BASE,
	.soc = SNAND_SOC_MT7988,
	.quad_spi = true
};

static void snand_gpio_clk_setup(void)
{
	/* Reset */
	mmio_setbits_32(0x10001080, 1 << 2);
	udelay(1000);
	mmio_setbits_32(0x10001084, 1 << 2);

	/* TOPCKGEN CFG0 nfi1x */
	mmio_write_32(CLK_CFG_3_CLR, CLK_NFI1X_SEL_MASK);
	mmio_write_32(CLK_CFG_3_SET, CLK_NFI1X_52MHz << CLK_NFI1X_SEL_S);

	/* TOPCKGEN CFG0 spinfi */
	mmio_write_32(CLK_CFG_3_CLR, CLK_SPINFI_BCLK_SEL_MASK);
	mmio_write_32(CLK_CFG_3_SET, CLK_NFI1X_52MHz << CLK_SPINFI_BCLK_SEL_S);

	mmio_write_32(CLK_CFG_UPDATE, NFI1X_CK_UPDATE | SPINFI_CK_UPDATE);

	/* GPIO mode */
	mmio_clrsetbits_32(0x1001F320, 0x77000000, 0x22000000);
	mmio_clrsetbits_32(0x1001F330, 0x7777, 0x2222);

	/* GPIO PUPD */
	mmio_clrsetbits_32(0x11D20070, 0xFC0000, 0x640000);
	mmio_clrsetbits_32(0x11D20090, 0xFC0000, 0x980000);
	mmio_clrsetbits_32(0x11D200B0, 0xFC0000, 0x640000);

	/* GPIO driving */
	mmio_clrsetbits_32(0x11D20010, 0x3F000000, 0x12000000);
	mmio_clrsetbits_32(0x11D20020, 0xFFF, 0x492);
}

const struct mtk_snand_platdata *mtk_plat_get_snfi_platdata(void)
{
	snand_gpio_clk_setup();

	return &mt7988_snand_pdata;
}

void mtk_plat_fip_location(size_t *fip_off, size_t *fip_size)
{
	*fip_off = FIP_BASE;
	*fip_size = FIP_SIZE;
}
