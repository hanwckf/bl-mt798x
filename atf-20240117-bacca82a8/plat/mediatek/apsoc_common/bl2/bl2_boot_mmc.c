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
#include <drivers/partition/mbr.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_block.h>
#include <drivers/mmc.h>
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

static const io_block_spec_t mmc_dev_gpt_spec = {
	.offset = 0 * MMC_BLOCK_SIZE,
	.length = 34 * MMC_BLOCK_SIZE,
};

static io_block_spec_t mmc_dev_bkup_gpt_spec = {
	.offset = 0 * MMC_BLOCK_SIZE,
	.length = 0, /* To be refilled by partition.c */
};

static io_block_spec_t mmc_dev_fip_spec;
static uintptr_t mmc_dev_handle;
static uint32_t num_sectors;

int mtk_mmc_gpt_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec,
			    uintptr_t *bkup_image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = mtk_plat_mmc_setup(&num_sectors);
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
	*bkup_image_spec = (uintptr_t)&mmc_dev_bkup_gpt_spec;

	return 0;
}

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const partition_entry_t *entry;
	int ret;

	ret = gpt_partition_init();
	if (ret != 0) {
		ERROR("Failed to initialize GPT partitions\n");
		return -ENOENT;
	}

	entry = get_partition_entry("fip");
	if (!entry) {
		ERROR("Partition 'fip' not found\n");
		return -ENOENT;
	}

	INFO("Located partition 'fip' at 0x%" PRIx64 ", size 0x%" PRIx64 "\n",
	     entry->start, entry->length);

	mmc_dev_fip_spec.offset = entry->start;
	mmc_dev_fip_spec.length = entry->length;

	*dev_handle = mmc_dev_handle;
	*image_spec = (uintptr_t)&mmc_dev_fip_spec;

	return 0;
}

void plat_patch_mbr_header(void *mbr)
{
	mbr_entry_t *mbr_entry;
	uint32_t num;

	/* mbr may not be 4-bytes aligned, so use a 4-bytes aligned address
	 * to access its member
	 */
	mbr_entry = (mbr_entry_t *)(mbr + MBR_PRIMARY_ENTRY_OFFSET);
	memcpy(&num, &mbr_entry->sector_nums, sizeof(uint32_t));
	if (num != num_sectors) {
		INFO("Patching MBR num of sectors: 0x%x -> 0x%x\n",
		     num, num_sectors - 1);

		num = num_sectors -1;
		memcpy(&mbr_entry->sector_nums, &num, sizeof(uint32_t));
	}
}
