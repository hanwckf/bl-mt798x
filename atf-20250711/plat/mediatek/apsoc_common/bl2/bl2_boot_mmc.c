// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
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
#include <mtk-sd.h>
#ifdef DUAL_FIP
#include "bsp_conf.h"
#include "dual_fip.h"
#endif

#define FIP_BOOT_OFFSET				0x100000

static size_t mmc_uda_read_blocks(int lba, uintptr_t buf, size_t size);

static io_block_dev_spec_t mmc_dev_uda_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = mmc_uda_read_blocks,
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

#ifdef DUAL_FIP
#define BSPCONF_ALIGNED_SIZE	\
	((sizeof(struct mtk_bsp_conf_data) + MMC_BLOCK_SIZE - 1) & ~(MMC_BLOCK_SIZE - 1))

#ifdef FIP_IN_BOOT0
static size_t mmc_boot0_read_blocks(int lba, uintptr_t buf, size_t size);

static io_block_dev_spec_t mmc_dev_boot0_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = mmc_boot0_read_blocks,
	},

	.block_size = MMC_BLOCK_SIZE,
};
#endif

#ifdef FIP2_IN_BOOT1
static size_t mmc_boot1_read_blocks(int lba, uintptr_t buf, size_t size);

static io_block_dev_spec_t mmc_dev_boot1_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = mmc_boot1_read_blocks,
	},

	.block_size = MMC_BLOCK_SIZE,
};
#endif

static io_block_spec_t mmc_dev_bootconf1_spec;
static io_block_spec_t mmc_dev_bootconf2_spec;
#endif

static io_block_spec_t mmc_dev_fip_spec;
#ifdef DUAL_FIP
static io_block_spec_t mmc_dev_fip2_spec;

static const uintptr_t mmc_dev_fips[] = {
	(uintptr_t)&mmc_dev_fip_spec,
	(uintptr_t)&mmc_dev_fip2_spec,
};

static io_block_spec_t mmc_dev_fip_curr_spec;

#ifdef FIP_IN_BOOT0
static uintptr_t mmc_dev_boot0_handle;
#endif
#ifdef FIP2_IN_BOOT1
static uintptr_t mmc_dev_boot1_handle;
#endif

static uintptr_t mmc_dev_handles[FIP_NUM];
static uintptr_t *mmc_dev_handle_ptr;
#endif

static uintptr_t mmc_dev_uda_handle;
static uint32_t num_sectors;

static int mtk_mmc_part_switch(uint8_t part)
{
	const char *part_name;
	int ret;

	static int8_t curr_part = -1;

	switch (part) {
	case 0:
		part_name = "UDA";
		break;

	case 1:
		part_name = "BOOT0";
		break;

	case 2:
		part_name = "BOOT1";
		break;

	default:
		return -EINVAL;
	}

	if (part == curr_part)
		return 0;

	ret = mmc_part_switch(part);
	if (ret < 0) {
		ERROR("Failed to switch to %s partition, %d\n", part_name, ret);
		return ret;
	}

	curr_part = part;

	return 0;
}

static size_t mmc_uda_read_blocks(int lba, uintptr_t buf, size_t size)
{
	int ret;

	if (mtk_mmc_device_type() == MMC_IS_EMMC) {
		ret = mtk_mmc_part_switch(0);
		if (ret)
			return 0;
	}

	return mmc_read_blocks(lba, buf, size);
}

static int mtk_mmc_gpt_init(void)
{
	int ret;

	static bool gpt_ready = false;

	if (gpt_ready)
		return 0;

	ret = gpt_partition_init();
	if (ret != 0) {
		ERROR("Failed to initialize GPT partitions\n");
		return -ENOENT;
	}

	gpt_ready = true;

	return 0;
}

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

	ret = io_dev_open(dev_con, (uintptr_t)&mmc_dev_uda_spec,
			  &mmc_dev_uda_handle);
	if (ret)
		return ret;

	*dev_handle = mmc_dev_uda_handle;
	*image_spec = (uintptr_t)&mmc_dev_gpt_spec;
	*bkup_image_spec = (uintptr_t)&mmc_dev_bkup_gpt_spec;

	return 0;
}

static uintptr_t fill_io_block_spec_gpt(io_block_spec_t *spec, const char *name)
{
	const partition_entry_t *entry;

	entry = get_partition_entry(name);
	if (!entry) {
		ERROR("Partition '%s' not found\n", name);
		return 0;
	}

	spec->offset = entry->start;
	spec->length = entry->length;

	NOTICE("Located partition '%s' at 0x%" PRIx64 ", size 0x%" PRIx64 "\n",
	       name, entry->start, entry->length);

	return mmc_dev_uda_handle;
}

#ifdef DUAL_FIP
static void mtk_load_bsp_conf_mmc(uintptr_t dev_handle)
{
	struct mtk_bsp_conf_data bc1, bc2, *pbc1 = NULL, *pbc2 = NULL;
	uint8_t buf[BSPCONF_ALIGNED_SIZE];
	const partition_entry_t *entry;
	int ret;

	entry = get_partition_entry(MTK_BSP_CONF_NAME);
	if (!entry) {
		ERROR("Partition '%s' not found\n", MTK_BSP_CONF_NAME);
		import_bsp_conf(pbc1, pbc2);
		return;
	}

	NOTICE("Located partition '%s' at 0x%" PRIx64 ", size 0x%" PRIx64 "\n",
	       MTK_BSP_CONF_NAME, entry->start, entry->length);

	mmc_dev_bootconf1_spec.offset = entry->start;
	mmc_dev_bootconf1_spec.length = BSPCONF_ALIGNED_SIZE;

	ret = mtk_dev_read_spec(dev_handle,
				(uintptr_t)&mmc_dev_bootconf1_spec,
				&buf, 0, sizeof(buf), MTK_BSP_CONF_NAME "1");
	if (!ret) {
		memcpy(&bc1, buf, sizeof(bc1));
		pbc1 = &bc1;
	}

	if (entry->length < 2 * BSPCONF_ALIGNED_SIZE) {
		ERROR("Partition '%s' is too small for redundancy\n",
		      MTK_BSP_CONF_NAME);
		import_bsp_conf(pbc1, NULL);
		return;
	}

	mmc_dev_bootconf2_spec.offset = entry->start + entry->length;
	mmc_dev_bootconf2_spec.offset -= BSPCONF_ALIGNED_SIZE;
	mmc_dev_bootconf2_spec.length = BSPCONF_ALIGNED_SIZE;

	ret = mtk_dev_read_spec(dev_handle,
				(uintptr_t)&mmc_dev_bootconf2_spec,
				&buf, 0, sizeof(buf), MTK_BSP_CONF_NAME "2");
	if (!ret) {
		memcpy(&bc2, buf, sizeof(bc2));
		pbc2 = &bc2;
	}

	import_bsp_conf(pbc1, pbc2);
}

#ifdef FIP_IN_BOOT0
static size_t mmc_boot0_read_blocks(int lba, uintptr_t buf, size_t size)
{
	int ret;

	ret = mtk_mmc_part_switch(1);
	if (ret)
		return 0;

	return mmc_read_blocks(lba, buf, size);
}

static int mtk_mmc_boot0_dev_setup(void)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = register_io_dev_block(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&mmc_dev_boot0_spec,
			  &mmc_dev_boot0_handle);
	if (ret)
		return ret;

	return 0;
}
#endif

#ifdef FIP2_IN_BOOT1
static size_t mmc_boot1_read_blocks(int lba, uintptr_t buf, size_t size)
{
	int ret;

	ret = mtk_mmc_part_switch(2);
	if (ret)
		return 0;

	return mmc_read_blocks(lba, buf, size);
}

static int mtk_mmc_boot1_dev_setup(void)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = register_io_dev_block(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&mmc_dev_boot1_spec,
			  &mmc_dev_boot1_handle);
	if (ret)
		return ret;

	return 0;
}
#endif

int mtk_dual_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	uint32_t slot;
	int ret;

	mtk_load_bsp_conf_mmc(mmc_dev_uda_handle);

#ifdef FIP_IN_BOOT0
	ret = mtk_mmc_boot0_dev_setup();
	if (ret)
		return ret;

	mmc_dev_fip_spec.offset = FIP_BOOT_OFFSET;
	mmc_dev_fip_spec.length = mmc_boot_part_size() - FIP_BOOT_OFFSET;

	NOTICE("FIP in BOOT0 at 0x%zx, size 0x%zx\n",
	       mmc_dev_fip_spec.offset, mmc_dev_fip_spec.length);

	mmc_dev_handles[0] = mmc_dev_boot0_handle;
#else
	mmc_dev_handles[0] = fill_io_block_spec_gpt(&mmc_dev_fip_spec, "fip");
#endif

#ifdef FIP2_IN_BOOT1
	ret = mtk_mmc_boot1_dev_setup();
	if (ret)
		return ret;

	mmc_dev_fip2_spec.offset = FIP_BOOT_OFFSET;
	mmc_dev_fip2_spec.length = mmc_boot_part_size() - FIP_BOOT_OFFSET;

	NOTICE("FIP2 in BOOT1 at 0x%zx, size 0x%zx\n",
	       mmc_dev_fip2_spec.offset, mmc_dev_fip2_spec.length);

	mmc_dev_handles[1] = mmc_dev_boot1_handle;
#else
	mmc_dev_handles[1] = fill_io_block_spec_gpt(&mmc_dev_fip2_spec, "fip2");
#endif

	ret = dual_fip_check(mmc_dev_handles, mmc_dev_fips, &slot);
	if (ret)
		return ret;

	mmc_dev_fip_curr_spec = *(io_block_spec_t *)mmc_dev_fips[slot];
	mmc_dev_handle_ptr = dev_handle;

	*dev_handle = mmc_dev_handles[slot];
	*image_spec = (uintptr_t)&mmc_dev_fip_curr_spec;

	return 0;
}

int mtk_fip_image_setup_next_slot(void)
{
	uint32_t slot;
	int ret;

	ret = dual_fip_next_slot(mmc_dev_handles, mmc_dev_fips, &slot);
	if (ret)
		return ret;

	*mmc_dev_handle_ptr = mmc_dev_handles[slot];
	mmc_dev_fip_curr_spec = *(io_block_spec_t *)mmc_dev_fips[slot];

	return 0;
}
#endif

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	int ret;

	ret = mtk_mmc_gpt_init();
	if (ret)
		return ret;

#ifdef DUAL_FIP
	return mtk_dual_fip_image_setup(dev_handle, image_spec);
#endif

	*dev_handle = fill_io_block_spec_gpt(&mmc_dev_fip_spec, "fip");
	if (!*dev_handle)
		return -ENOENT;

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
