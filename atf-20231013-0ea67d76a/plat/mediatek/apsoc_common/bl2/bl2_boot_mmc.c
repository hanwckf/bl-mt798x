/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <inttypes.h>
#include <common/debug.h>
#include <common/tbbr/tbbr_img_def.h>
#include <drivers/partition/partition.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_block.h>
#include <drivers/mmc.h>
#include <drivers/mmc/mtk-sd.h>
#include "bl2_plat_setup.h"

static io_block_dev_spec_t mmc_dev_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = mmc_read_blocks,
	},

	.block_size = MMC_BLOCK_SIZE,
};

static io_block_spec_t mmc_dev_gpt_spec = {
	.offset = 0 * MMC_BLOCK_SIZE,
	.length = 34 * MMC_BLOCK_SIZE,
};

static io_block_spec_t mmc_dev_fip_spec;
static uintptr_t mmc_dev_handle;

int mtk_mmc_gpt_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = mtk_plat_mmc_setup();
	if (ret)
		return ret;

	ret = register_io_dev_block(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&mmc_dev_spec, &mmc_dev_handle);
	if (ret)
		return ret;

	*dev_handle = mmc_dev_handle;
	*image_spec = (uintptr_t)&mmc_dev_gpt_spec;

	return 0;
}

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const partition_entry_t *entry;
	uint64_t uda_size;

	partition_init(GPT_IMAGE_ID);

	entry = get_partition_entry("fip");
	if (!entry) {
		INFO("Partition 'fip' not found in parimary GPT\n");

		uda_size = mtk_mmc_device_size();
		mmc_dev_gpt_spec.offset = uda_size - 33 * MMC_BLOCK_SIZE;
		mmc_dev_gpt_spec.length = 33 * MMC_BLOCK_SIZE;

		partition_init_secondary_gpt(GPT_IMAGE_ID);
		entry = get_partition_entry("fip");
		if (!entry) {
			ERROR("Partition 'fip' not found in secondary GPT\n");
			return -ENOENT;
		}
	}

	INFO("Located partition 'fip' at 0x%" PRIx64 ", size 0x%" PRIx64 "\n",
	     entry->start, entry->length);

	mmc_dev_fip_spec.offset = entry->start;
	mmc_dev_fip_spec.length = entry->length;

	*dev_handle = mmc_dev_handle;
	*image_spec = (uintptr_t)&mmc_dev_fip_spec;

	return 0;
}
