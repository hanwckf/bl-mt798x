/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/mmc.h>
#include <drivers/mmc/mtk-sd.h>
#include <lib/mmio.h>
#include <mt7988_gpio.h>
#include <reg_base.h>

struct msdc_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
	const uint32_t *pupd;
};

static const struct msdc_compatible mt7988_msdc_compat = {
	.clk_div_bits = 12,
	.pad_tune0 = true,
	.async_fifo = true,
	.data_tune = true,
	.busy_check = true,
	.stop_clk_fix = true,
};

static const uint32_t msdc0_pins[] = {
	GPIO38, GPIO39, GPIO40, GPIO41, GPIO42, GPIO43,
	GPIO44, GPIO45, GPIO46, GPIO47, GPIO48, GPIO49,
};

static const uint32_t msdc1_pins[] = {
	GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, GPIO37,
};

static const uint32_t msdc0_pupd[] = {
	MT_GPIO_PULL_UP, MT_GPIO_PULL_DOWN, MT_GPIO_PULL_DOWN, MT_GPIO_PULL_UP,
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP,   MT_GPIO_PULL_UP,   MT_GPIO_PULL_UP,
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP,   MT_GPIO_PULL_UP,   MT_GPIO_PULL_UP
};

static const uint32_t msdc1_pupd[] = {
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_UP,
	MT_GPIO_PULL_UP, MT_GPIO_PULL_UP, MT_GPIO_PULL_DOWN,
};

static const struct msdc_gpio_mux_info msdc0_pinmux = {
	.pins = msdc0_pins,
	.count = ARRAY_SIZE(msdc0_pins),
	.mux = 1,
	.pupd = msdc0_pupd,
};

static const struct msdc_gpio_mux_info msdc1_pinmux = {
	.pins = msdc1_pins,
	.count = ARRAY_SIZE(msdc1_pins),
	.mux = 5,
	.pupd = msdc1_pupd,
};

static const struct mt7988_msdc_conf {
	uintptr_t base;
	uintptr_t top_base;
	uint32_t bus_width;
	enum mmc_device_type type;
	uint32_t src_clk;
	const struct msdc_compatible *dev_comp;
	const struct msdc_gpio_mux_info *pinmux;
} mt7988_msdc[] = {
	{
		.base = MSDC0_BASE,
		.top_base = MSDC0_TOP_BASE,
		.bus_width = MMC_BUS_WIDTH_8,
		.type = MMC_IS_EMMC,
		.src_clk = 384000000,
		.dev_comp = &mt7988_msdc_compat,
		.pinmux = &msdc0_pinmux,
	},
	{
		.base = MSDC0_BASE,
		.top_base = MSDC0_TOP_BASE,
		.bus_width = MMC_BUS_WIDTH_4,
		.type = MMC_IS_SD,
		.src_clk = 384000000,
		.dev_comp = &mt7988_msdc_compat,
		.pinmux = &msdc1_pinmux,
	}
};

static void mmc_gpio_setup(void)
{
	/* eMMC boot */
	if (MSDC_INDEX == 0) {
		/* GPIO IES */
		mmio_clrsetbits_32(MSDC_GPIO_IES_CFG0, 0xFFF, 0xFFF);

		/* GPIO SMT */
		mmio_clrsetbits_32(MSDC_GPIO_SMT_CFG0, 0xFFF, 0xFFF);

		/* GPIO R0/R1 */
		mmio_clrsetbits_32(MSDC_GPIO_R0_CFG0, 0xFFF, 0xBFE);
		mmio_clrsetbits_32(MSDC_GPIO_R1_CFG0, 0xFFF, 0x401);

		/* GPIO driving */
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG0, 0x3FFFFFFF, 0x9249249);
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG1, 0x3F, 0x9);

		/* RDSEL */
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG0, 0x3FFFFFFF, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG1, 0x3FFFFFFF, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_RDSEL_CFG2, 0xFFF, 0x0);

		/* TDSEL */
		mmio_clrsetbits_32(MSDC_GPIO_TDSEL_CFG0, 0xFFFFFFFF, 0x0);
		mmio_clrsetbits_32(MSDC_GPIO_TDSEL_CFG1, 0xFFFF, 0x0);
	} else {
		/* IES */
		mmio_clrsetbits_32(SD_GPIO_IES_CFG0, 0xF0000000, 0xF0000000);
		mmio_clrsetbits_32(SD_GPIO_IES_CFG1, 0x3, 0x3);

		/* SMT  */
		mmio_clrsetbits_32(SD_GPIO_SMT_CFG0, 0xF0000000, 0xF0000000);
		mmio_clrsetbits_32(SD_GPIO_SMT_CFG1, 0x3, 0x3);

		/* GPIO R0/R1 */
		mmio_clrsetbits_32(SD_GPIO_R0_CFG0, 0xF0000000, 0xF0000000);
		mmio_clrsetbits_32(SD_GPIO_R0_CFG1, 0x3, 0x1);
		mmio_clrsetbits_32(SD_GPIO_R1_CFG0, 0xF0000000, 0x0);
		mmio_clrsetbits_32(SD_GPIO_R1_CFG1, 0x3, 0x2);

		/* DRIVING  */
		mmio_clrsetbits_32(SD_GPIO_DRV_CFG2, 0x3F000000, 0x9000000);
		mmio_clrsetbits_32(SD_GPIO_DRV_CFG3, 0xFFF, 0x249);

		/* RDSEL */
		mmio_clrsetbits_32(SD_GPIO_RDSEL_CFG5, 0x3FFC0000, 0x0);
		mmio_clrsetbits_32(SD_GPIO_RDSEL_CFG6, 0xFFFFFF, 0x0);

		/* TDSEL */
		mmio_clrsetbits_32(SD_GPIO_TDSEL_CFG3, 0xFFFF0000, 0x0);
		mmio_clrsetbits_32(SD_GPIO_TDSEL_CFG4, 0xFF, 0x0);
	}
}

int mtk_plat_mmc_setup(uint32_t *num_sectors)
{
	const struct mt7988_msdc_conf *conf = &mt7988_msdc[MSDC_INDEX];
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
