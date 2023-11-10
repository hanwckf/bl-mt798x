/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <inttypes.h>
#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_ubi.h>
#include <drivers/nand.h>
#include "bl2_plat_setup.h"

#ifdef OVERRIDE_UBI_START_ADDR
#define UBI_START_ADDR			OVERRIDE_UBI_START_ADDR
#ifdef OVERRIDE_UBI_END_ADDR
#define UBI_END_ADDR			OVERRIDE_UBI_END_ADDR
#else
#define UBI_END_ADDR			0
#endif
#else
#define UBI_START_ADDR			0x200000
#define UBI_END_ADDR			0
#endif

static int nand_ubispl_is_bad_block(uint32_t pnum)
{
	struct nand_device *nand_dev = get_nand_device();
	int ret;

	ret = nand_dev->mtd_block_is_bad(pnum);
	if (ret)
		return -EIO;

	return 0;
}

static int nand_ubispl_read(uint32_t pnum, unsigned long offset,
			    unsigned long len, void *dst)
{
	struct nand_device *nand_dev = get_nand_device();
	size_t len_read;
	uint64_t addr;
	int ret;

	addr = (uint64_t)pnum * nand_dev->block_size + offset;

	ret = nand_read(addr, (uintptr_t)dst, len, &len_read);
	if (ret < 0) {
		ERROR("nand_read(%" PRIu64 ") failed with %d. %zu bytes read\n",
		      addr, ret, len_read);
		return ret;
	}

	return 0;
}

static io_ubi_dev_spec_t nand_ubi_dev_spec = {
	.ubi = (struct ubi_scan_info *)SCRATCH_BUF_OFFSET,
	.is_bad_peb = nand_ubispl_is_bad_block,
	.read = nand_ubispl_read,
	.fastmap = 0,
};

static const io_ubi_spec_t ubi_dev_fip_spec = {
	.vol_id = -1,
	.vol_name = "fip",
};

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	size_t page_size, block_size;
	uint64_t nand_size;
	int ret;

	ret = mtk_plat_nand_setup(&page_size, &block_size, &nand_size);
	if (ret)
		return ret;

	nand_ubi_dev_spec.peb_size = block_size;
	nand_ubi_dev_spec.peb_offset = UBI_START_ADDR / block_size;
	nand_ubi_dev_spec.peb_count = ((UBI_END_ADDR ?: nand_size) - UBI_START_ADDR) /
				      block_size;
	nand_ubi_dev_spec.vid_offset = page_size;
	nand_ubi_dev_spec.leb_start = page_size * 2;

	ret = register_io_dev_ubi(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&nand_ubi_dev_spec, dev_handle);
	if (ret)
		return ret;

	*image_spec = (uintptr_t)&ubi_dev_fip_spec;

	return 0;
}
