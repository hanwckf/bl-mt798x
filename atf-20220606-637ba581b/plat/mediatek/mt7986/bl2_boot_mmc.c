/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/io/io_driver.h>
#include <drivers/mmc.h>
#include <drivers/mmc/mtk-sd.h>
#include <drivers/partition/partition.h>
#include <common/tbbr/tbbr_img_def.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <bl2_boot_dev.h>
#include <mt7986_gpio.h>

#define FIP_BASE			0x680000
#define FIP_SIZE			0x200000

struct msdc_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
	const uint32_t *pupd;
};

static const struct msdc_compatible mt7986_msdc0_compat = {
	.clk_div_bits = 12,
	.pad_tune0 = true,
	.async_fifo = true,
	.data_tune = true,
	.busy_check = true,
	.stop_clk_fix = true,
	.enhance_rx = true,
};

/* eMMC51 DAT0~7 is GPIO50~57, CMD is GPIO58, CLK is GPIO59, DSL is GPIO60, RSTB is GPIO61 */
/* eMMC45 DAT0~7 is GPIO23~30, CMD is GPIO31, CLK is GPIO32, RSTB is GPIO22 */

static const struct mt7986_msdc_conf {
	uintptr_t base;
	uintptr_t top_base;
	uint32_t bus_width;
	enum mmc_device_type type;
	uint32_t src_clk;
	const struct msdc_compatible *dev_comp;
} mt7986_msdc[] = {
	{
		.base = MSDC0_BASE,
		.top_base = MSDC0_TOP_BASE,
		.bus_width = MMC_BUS_WIDTH_8,
		.type = MMC_IS_EMMC,
		.src_clk = 40000000,
		.dev_comp = &mt7986_msdc0_compat,
	},
	{
		.base = MSDC0_BASE,
		.top_base = MSDC0_TOP_BASE,
		.bus_width = MMC_BUS_WIDTH_4,
		.type = MMC_IS_SD,
		.src_clk = 40000000,
		.dev_comp = &mt7986_msdc0_compat,
	}
};

static const io_block_dev_spec_t mmc_dev_spec = {
	.buffer = {
		.offset = 0x41000000,
		.length = 0x1000000,
	},
	.ops = {
		.read = mmc_read_blocks,
	},
	.block_size = MMC_BLOCK_SIZE,
};

const io_block_spec_t mtk_boot_dev_gpt_spec = {
	.offset	= 0 * MMC_BLOCK_SIZE,
	.length = 34 * MMC_BLOCK_SIZE,
};

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset	= FIP_BASE,
	.length = FIP_SIZE,
};

static void mmc_gpio_setup(void)
{
	if (mmio_read_32(0x11d00a0c) == 1) {
		/* eMMC51 DAT0~7 is GPIO50~57,
		 * CMD is GPIO58, CLK is GPIO59,
		 * DSL is GPIO60, RSTB is GPIO61 */
		mmio_clrsetbits_32(GPIO_MODE6,
			0x7 << GPIO_PIN50_S | 0x7 << GPIO_PIN51_S |
			0x7 << GPIO_PIN52_S | 0x7 << GPIO_PIN53_S |
			0x7 << GPIO_PIN54_S | 0x7 << GPIO_PIN55_S,
			0x1 << GPIO_PIN50_S | 0x1 << GPIO_PIN51_S |
			0x1 << GPIO_PIN52_S | 0x1 << GPIO_PIN53_S |
			0x1 << GPIO_PIN54_S | 0x1 << GPIO_PIN55_S);
		mmio_clrsetbits_32(GPIO_MODE7,
			0x7 << GPIO_PIN56_S | 0x7 << GPIO_PIN57_S |
			0x7 << GPIO_PIN58_S | 0x7 << GPIO_PIN59_S |
			0x7 << GPIO_PIN60_S | 0x7 << GPIO_PIN61_S,
			0x1 << GPIO_PIN56_S | 0x1 << GPIO_PIN57_S |
			0x1 << GPIO_PIN58_S | 0x1 << GPIO_PIN59_S |
			0x1 << GPIO_PIN60_S | 0x1 << GPIO_PIN61_S);

		/* GPIO IES */
		mmio_clrsetbits_32(MSDC_GPIO_IES_CFG0,
			0xfff << EMMC51_GPIO_IES_S, 0xfff << EMMC51_GPIO_IES_S);

		/* GPIO SMT */
		mmio_clrsetbits_32(MSDC_GPIO_SMT_CFG0,
			0xfff << EMMC51_GPIO_SMT_S, 0xfff << EMMC51_GPIO_SMT_S);

		/* GPIO pupd */
		mmio_clrsetbits_32(MSDC_GPIO_PUPD_CFG0,
			0xfff << EMMC51_GPIO_PUPD_S, 0x401 << EMMC51_GPIO_PUPD_S);
		/* GPIO R0 */
		mmio_clrsetbits_32(MSDC_GPIO_R0_CFG0,
			0xfff << EMMC51_GPIO_R0_S, 0xBFE << EMMC51_GPIO_R0_S);

		/* GPIO R1 */
		mmio_clrsetbits_32(MSDC_GPIO_R1_CFG0,
			0xfff << EMMC51_GPIO_R1_S, 0x401 << EMMC51_GPIO_R1_S);

		/* GPIO driving */
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG0,
			0x7 << EMMC51_CLK_DRV_S | 0x7 << EMMC51_CMD_DRV_S |
			0x7 << EMMC51_DAT0_DRV_S | 0x7 << EMMC51_DAT1_DRV_S |
			0x7 << EMMC51_DAT2_DRV_S | 0x7 << EMMC51_DAT3_DRV_S |
			0x7 << EMMC51_DAT4_DRV_S | 0x7 << EMMC51_DAT5_DRV_S |
			0x7 << EMMC51_DAT6_DRV_S | 0x7 << EMMC51_DAT7_DRV_S,
			0x2 << EMMC51_CLK_DRV_S | 0x1 << EMMC51_CMD_DRV_S |
			0x1 << EMMC51_DAT0_DRV_S | 0x1 << EMMC51_DAT1_DRV_S |
			0x1 << EMMC51_DAT2_DRV_S | 0x1 << EMMC51_DAT3_DRV_S |
			0x1 << EMMC51_DAT4_DRV_S | 0x1 << EMMC51_DAT5_DRV_S |
			0x1 << EMMC51_DAT6_DRV_S | 0x1 << EMMC51_DAT7_DRV_S);
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG1,
			0x7 << EMMC51_DSL_DRV_S | 0x7 << EMMC51_RSTB_DRV_S,
			0x1 << EMMC51_DSL_DRV_S | 0x1 << EMMC51_RSTB_DRV_S);
	} else {
		/* eMMC45 DAT0~7 is GPIO23~30,
		 * CMD is GPIO31, CLK is GPIO32,
		 * RSTB is GPIO22 */
		mmio_clrsetbits_32(GPIO_MODE2,
			0x7 << GPIO_PIN22_S | 0x7 << GPIO_PIN23_S,
			0x2 << GPIO_PIN22_S | 0x2 <<GPIO_PIN23_S);
		mmio_clrsetbits_32(GPIO_MODE3,
			0x7 << GPIO_PIN24_S | 0x7 << GPIO_PIN25_S |
			0x7 << GPIO_PIN26_S | 0x7 << GPIO_PIN27_S |
			0x7 << GPIO_PIN28_S | 0x7 << GPIO_PIN29_S |
			0x7 << GPIO_PIN30_S | 0x7 << GPIO_PIN31_S,
			0x2 << GPIO_PIN24_S | 0x2 << GPIO_PIN25_S |
			0x2 << GPIO_PIN26_S | 0x2 << GPIO_PIN27_S |
			0x2 << GPIO_PIN28_S | 0x2 << GPIO_PIN29_S |
			0x2 << GPIO_PIN30_S | 0x2 << GPIO_PIN31_S);
		mmio_clrsetbits_32(GPIO_MODE4,
			0x7 << GPIO_PIN32_S, 0x2 << GPIO_PIN32_S);

		/* GPIO IES */
		mmio_clrsetbits_32(MSDC_GPIO_IES_CFG0,
			0x7ff << EMMC45_GPIO_IES_S, 0x7ff << EMMC45_GPIO_IES_S);

		/* GPIO SMT */
		mmio_clrsetbits_32(MSDC_GPIO_SMT_CFG0,
			0x7ff << EMMC45_GPIO_SMT_S, 0x7ff << EMMC45_GPIO_SMT_S);

		/* GPIO pupd */
		mmio_clrsetbits_32(MSDC_GPIO_PUPD_CFG0,
			0x7ff << EMMC45_GPIO_PUPD_S, 0x100 << EMMC45_GPIO_PUPD_S);

		/* GPIO R0 */
		mmio_clrsetbits_32(MSDC_GPIO_R0_CFG0,
			0x7ff << EMMC45_GPIO_R0_S, 0x6ff << EMMC45_GPIO_R0_S);

		/* GPIO R1 */
		mmio_clrsetbits_32(MSDC_GPIO_R1_CFG0,
			0x7ff << EMMC45_GPIO_R1_S, 0x100 << EMMC45_GPIO_R1_S);

		/* GPIO driving */
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG1,
			0x7 << EMMC45_RSTB_DRV_S | 0x7 << EMMC45_DAT0_DRV_S |
			0x7 << EMMC45_DAT3_DRV_S | 0x7 << EMMC45_DAT4_DRV_S |
			0x7 << EMMC45_DAT2_DRV_S | 0x7 << EMMC45_DAT1_DRV_S |
			0x7 << EMMC45_DAT5_DRV_S,
			0x1 << EMMC45_RSTB_DRV_S | 0x1 << EMMC45_DAT0_DRV_S |
			0x1 << EMMC45_DAT3_DRV_S | 0x1 << EMMC45_DAT4_DRV_S |
			0x1 << EMMC45_DAT2_DRV_S | 0x1 << EMMC45_DAT1_DRV_S |
			0x1 << EMMC45_DAT5_DRV_S);
		mmio_clrsetbits_32(MSDC_GPIO_DRV_CFG2,
			0x7 << EMMC45_DAT6_DRV_S | 0x7 << EMMC45_CLK_DRV_S |
			0x7 << EMMC45_CMD_DRV_S | 0x7 << EMMC45_DAT7_DRV_S,
			0x1 << EMMC45_DAT6_DRV_S | 0x2 << EMMC45_CLK_DRV_S |
			0x1 << EMMC45_CMD_DRV_S | 0x1 << EMMC45_DAT7_DRV_S);
	}
}

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	int result;
	const struct mt7986_msdc_conf *conf = &mt7986_msdc[MSDC_INDEX];

	mmc_gpio_setup();

	mtk_mmc_init(conf->base, conf->top_base, conf->dev_comp,
		     conf->src_clk, conf->type, conf->bus_width);

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&mmc_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
