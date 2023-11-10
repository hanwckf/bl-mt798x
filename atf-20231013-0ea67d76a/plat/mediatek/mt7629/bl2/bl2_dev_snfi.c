/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <platform_def.h>
#include <pinctrl.h>
#include <mtk-snand.h>

#define FIP_BASE			0x80000
#define FIP_SIZE			0x200000

struct snfi_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
};

static const uint32_t snfi_pins[] = { 62, 63, 64, 65, 66, 67 };

static const struct snfi_gpio_mux_info snfi_pinmux = {
	.pins = snfi_pins,
	.count = ARRAY_SIZE(snfi_pins),
	.mux = 2,
};

static const struct mtk_snand_platdata mt7629_snand_pdata = {
	.nfi_base = (void *)NFI_BASE,
	.ecc_base = (void *)NFIECC_BASE,
	.soc = SNAND_SOC_MT7629,
	.quad_spi = true
};

const struct mtk_snand_platdata *mtk_plat_get_snfi_platdata(void)
{
	uint32_t i;

	for (i = 0; i < snfi_pinmux.count; i++)
		mtk_set_pin_mode(snfi_pinmux.pins[i], snfi_pinmux.mux);

	return &mt7629_snand_pdata;
}

void mtk_plat_fip_location(size_t *fip_off, size_t *fip_size)
{
	*fip_off = FIP_BASE;
	*fip_size = FIP_SIZE;
}
