// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <inttypes.h>
#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_block.h>
#include <drivers/nand.h>
#include "bl2_plat_setup.h"

static io_block_spec_t nand_dev_fip_spec;

/* Adjust offset based on bad blocks start from beginning of a partition */
static int nand_adjust_offset(uint64_t base, uint64_t limit, uint64_t offset,
			      uint64_t *result)
{
	struct nand_device *nand_dev = get_nand_device();
	uint64_t addr, end, end_lim, len_left;
	uint32_t boffs, erasesize_mask;
	int ret;

	VERBOSE("Adjusting offset, base: 0x%08" PRIx64 ", limit: 0x%08" PRIx64 ", offset: 0x%08" PRIx64 "\n",
		base, limit, offset);

	erasesize_mask = nand_dev->block_size - 1;

	if (limit)
		end_lim = limit;
	else
		end_lim = nand_dev->size;

	addr = base & (~erasesize_mask);
	end = base + offset;
	boffs = end & erasesize_mask;
	end &= ~erasesize_mask;
	len_left = end - addr;

	while (addr < end_lim) {
		ret = nand_dev->mtd_block_is_bad(addr / nand_dev->block_size);
		if (ret > 0) {
			VERBOSE("Skipping bad block 0x%08" PRIx64 "\n", addr);
			addr += nand_dev->block_size;
			continue;
		} else if (ret < 0) {
			ERROR("mtd_block_is_bad(0x%08" PRIx64 ") failed\n",
			      addr);
			return ret;
		}

		if (!len_left)
			break;

		addr += nand_dev->block_size;
		len_left -= nand_dev->block_size;
	}

	if (len_left) {
		ERROR("Incomplete offset adjustment end at 0x%08" PRIx64 ", 0x%" PRIx64 " left\n",
		      addr, len_left);
		return -EIO;
	}

	VERBOSE("Offset 0x%08" PRIx64 " adjusted to 0x%08" PRIx64 "\n", offset,
		addr + boffs - base);

	if (result)
		*result = addr + boffs - base;

	return 0;
}

static size_t nand_read_range(int lba, uintptr_t buf, size_t size)
{
	struct nand_device *nand_dev = get_nand_device();
	size_t length_read = 0;
	uint64_t off;
	int ret;

	off = (uint64_t)lba * nand_dev->page_size;

	if (off >= nand_dev_fip_spec.offset &&
	    off < nand_dev_fip_spec.offset + nand_dev_fip_spec.length) {
		ret = nand_adjust_offset(nand_dev_fip_spec.offset,
					 nand_dev_fip_spec.offset + nand_dev_fip_spec.length,
					 off - nand_dev_fip_spec.offset, &off);
		if (ret)
			return 0;

		off += nand_dev_fip_spec.offset;
	}

	ret = nand_read(off, buf, size, &length_read);
	if (ret < 0) {
		ERROR("nand_read(%" PRIx64 ") failed with %d. %zu bytes read\n",
		      off, ret, length_read);
	}

	return length_read;
}

static io_block_dev_spec_t nand_dev_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = nand_read_range,
	},
};

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = mtk_plat_nand_setup(&nand_dev_spec.block_size, NULL, NULL);
	if (ret)
		return ret;

	mtk_fip_location(&nand_dev_fip_spec.offset, &nand_dev_fip_spec.length);

	ret = register_io_dev_block(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&nand_dev_spec, dev_handle);
	if (ret)
		return ret;

	*image_spec = (uintptr_t)&nand_dev_fip_spec;

	return 0;
}
