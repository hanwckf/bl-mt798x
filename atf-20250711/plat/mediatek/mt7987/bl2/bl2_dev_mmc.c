// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <assert.h>
#include <drivers/mmc.h>
#include <drivers/mmc/mtk-sd.h>
#include <lib/mmio.h>
#include <mt7987_gpio.h>
#include <reg_base.h>

struct msdc_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
	const uint32_t *pupd;
};

static const struct msdc_compatible mt7987_msdc_compat = {
	.clk_div_bits = 12,
	.pad_tune0 = true,
	.async_fifo = true,
	.async_fifo_crcsts = true,
	.data_tune = true,
	.busy_check = true,
	.stop_clk_fix = true,
	.enhance_rx = true,
};

static const uint32_t msdc0_pins[] = {
	GPIO14, GPIO15, GPIO16, GPIO17, GPIO18, GPIO19,
	GPIO20, GPIO21, GPIO22, GPIO23, GPIO24,
};

static const uint32_t msdc1_pins[] = {
	GPIO15, GPIO16, GPIO17, GPIO18, GPIO23, GPIO24,
};

static const uint32_t msdc0_pupd[] = {
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_UP,
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_UP,
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_DOWN
};

static const uint32_t msdc1_pupd[] = {
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_UP,
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_DOWN,
};

static const struct msdc_gpio_mux_info msdc0_pinmux = {
	.pins = msdc0_pins,
	.count = ARRAY_SIZE(msdc0_pins),
	.mux = 2,
	.pupd = msdc0_pupd,
};

static const struct msdc_gpio_mux_info msdc1_pinmux = {
	.pins = msdc1_pins,
	.count = ARRAY_SIZE(msdc1_pins),
	.mux = 2,
	.pupd = msdc1_pupd,
};

static const struct mt7987_msdc_conf {
	uintptr_t base;
	uintptr_t top_base;
	uint32_t bus_width;
	enum mmc_device_type type;
	uint32_t src_clk;
	const struct msdc_compatible *dev_comp;
	const struct msdc_gpio_mux_info *pinmux;
} mt7987_msdc[] = {
	{
		.base = MSDC0_BASE,
		.top_base = MSDC0_TOP_BASE,
		.bus_width = MMC_BUS_WIDTH_8,
		.type = MMC_IS_EMMC,
		.src_clk = 200000000,
		.dev_comp = &mt7987_msdc_compat,
		.pinmux = &msdc0_pinmux,
	},
	{
		.base = MSDC0_BASE,
		.top_base = MSDC0_TOP_BASE,
		.bus_width = MMC_BUS_WIDTH_4,
		.type = MMC_IS_SD,
		.src_clk = 200000000,
		.dev_comp = &mt7987_msdc_compat,
		.pinmux = &msdc1_pinmux,
	}
};

static void mmc_gpio_setup(void)
{
	/* eMMC boot */
	if (MSDC_INDEX == 0) {
		/* GPIO IES */
		mmio_clrsetbits_32(MSDC_GPIO_IES_CFG0, 0x9FF8, 0x9FF8);

		/* GPIO SMT */
		mmio_clrsetbits_32(MSDC_GPIO_SMT_CFG0, 0x9FF8, 0x9FF8);

		/* GPIO R0/R1 */
		mmio_clrsetbits_32(MSDC_GPIO_R0_CFG0, 0x9FF8, 0x9BF8);
		mmio_clrsetbits_32(MSDC_GPIO_R1_CFG0, 0x9FF8, 0x400);

		/* GPIO driving */
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG0, 0x3FFFFE00, 0x9249200);
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG1, 0x381FF, 0x8049);

		/* RDSEL */
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG0, 0x3FFC0000, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG1, 0x3FFFFFFF, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG2, 0x3FFFF, 0x0);
                mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG3, 0x3F, 0x0);

		/* TDSEL */
		mmio_clrsetbits_32(MSDC_GPIO_TDSEL_CFG0, 0xFFFFF000, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_TDSEL_CFG1, 0xF00FFFFF, 0x0);
	} else {
		/* GPIO IES */
		mmio_clrsetbits_32(MSDC_GPIO_IES_CFG0, 0xCD8, 0xCD8);

		/* GPIO SMT */
		mmio_clrsetbits_32(MSDC_GPIO_SMT_CFG0, 0xCD8, 0xCD8);

		/* GPIO R0/R1 */
                mmio_clrsetbits_32(MSDC_GPIO_R0_CFG0, 0xCD8, 0x8D8);
		mmio_clrsetbits_32(MSDC_GPIO_R1_CFG0, 0xCD8, 0x400);

		/* GPIO driving */
                mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG0, 0xFC7E00, 0x241200);
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG1, 0x3F, 0x9);

		/* RDSEL */
                mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG0, 0x3FFC0000, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG1, 0x3FFC0, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG2, 0xFFF, 0x0);

		/* TDSEL */
                mmio_clrsetbits_32(MSDC_GPIO_TDSEL_CFG0, 0xFF0FF000, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_TDSEL_CFG1, 0xFF00, 0x0);
	}
}

int mtk_plat_mmc_setup(uint32_t *num_sectors)
{
	const struct mt7987_msdc_conf *conf = &mt7987_msdc[MSDC_INDEX];
	uint32_t i;

	for (i = 0; i < conf->pinmux->count; i++) {
		mt_set_pinmux_mode(conf->pinmux->pins[i], conf->pinmux->mux);
		mt_set_gpio_pull(conf->pinmux->pins[i], conf->pinmux->pupd[i]);
	}

	mmc_gpio_setup();

	mtk_mmc_init(conf->base, conf->top_base, conf->dev_comp, conf->src_clk,
		     conf->type, conf->bus_width);

	if (num_sectors)
		*num_sectors = mtk_mmc_block_count();

	return 0;
}
