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
#include <pinctrl.h>

struct msdc_gpio_mux_info {
	const uint32_t *pins;
	uint32_t count;
	uint32_t mux;
};

static const struct msdc_compatible mt7622_msdc0_compat = {
	.clk_div_bits = 12,
	.pad_tune0 = true,
	.async_fifo = true,
	.data_tune = true,
	.busy_check = true,
	.stop_clk_fix = true,
};

static const struct msdc_compatible mt7622_msdc1_compat = {
	.clk_div_bits = 12,
	.pad_tune0 = true,
	.async_fifo = true,
	.data_tune = true,
	.busy_check = true,
	.stop_clk_fix = true,
	.r_smpl = true,
};

static const uint32_t msdc0_pins[] = { 40, 41, 42, 43, 44, 45, 47, 48, 49, 50 };
static const uint32_t msdc1_pins[] = { 16, 17, 18, 19, 20, 21 };

static const struct msdc_gpio_mux_info msdc0_pinmux = {
	.pins = msdc0_pins,
	.count = ARRAY_SIZE(msdc0_pins),
	.mux = 2,
};

static const struct msdc_gpio_mux_info msdc1_pinmux = {
	.pins = msdc1_pins,
	.count = ARRAY_SIZE(msdc1_pins),
	.mux = 2,
};

static const struct mt7622_msdc_conf {
	uintptr_t base;
	uint32_t bus_width;
	enum mmc_device_type type;
	uint32_t src_clk;
	const struct msdc_compatible *dev_comp;
	const struct msdc_gpio_mux_info *pinmux;
} mt7622_msdc[] = {
	{
		.base = MSDC0_BASE,
		.bus_width = MMC_BUS_WIDTH_8,
		.type = MMC_IS_EMMC,
		.src_clk = 48000000,
		.dev_comp = &mt7622_msdc0_compat,
		.pinmux = &msdc0_pinmux,
	}, {
		.base = MSDC1_BASE,
		.bus_width = MMC_BUS_WIDTH_4,
		.type = MMC_IS_SD,
		.src_clk = 48000000,
		.dev_comp = &mt7622_msdc1_compat,
		.pinmux = &msdc1_pinmux,
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
};

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	const struct mt7622_msdc_conf *conf = &mt7622_msdc[MSDC_INDEX];
	const partition_entry_t *entry;
	io_block_spec_t *fip_desc = (io_block_spec_t *)&mtk_boot_dev_fip_spec;
	int i, result;

	for (i = 0; i < conf->pinmux->count; i++)
		mtk_set_pin_mode(conf->pinmux->pins[i], conf->pinmux->mux);

	mtk_mmc_init(conf->base, 0, conf->dev_comp, conf->src_clk, conf->type,
		     conf->bus_width);

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&mmc_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	partition_init(GPT_IMAGE_ID);

	entry = get_partition_entry("fip");
	if (!entry) {
		ERROR("Partition 'fip' not found\n");
		panic();
	}

	INFO("Located GPT partition 'fip' at 0x%llx, size 0x%llx\n",
	       entry->start, entry->length);

	fip_desc->offset = entry->start;
	fip_desc->length = entry->length;

	/* Ignore improbable errors in release builds */
	(void)result;
}
