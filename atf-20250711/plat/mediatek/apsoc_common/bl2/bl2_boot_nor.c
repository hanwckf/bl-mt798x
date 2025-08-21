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
#include <drivers/spi_nor.h>
#include "bl2_plat_setup.h"

#define READ_CHKSZ	0x1000

static size_t nor_read_range(int lba, uintptr_t buf, size_t size)
{
	size_t retlen, chksz, len_left = size;
	int ret;

	while (len_left) {
		chksz = len_left > READ_CHKSZ ? READ_CHKSZ : len_left;
		ret = spi_nor_read(lba, buf, chksz, &retlen);
		if (ret < 0) {
			ERROR("spi_nor_read(%u) failed with %d. %zu bytes read\n",
			      lba, ret, retlen);
			return size - len_left;
		}

		buf += chksz;
		lba += chksz;
		len_left -= chksz;
	}

	return size;
}

static io_block_dev_spec_t nor_dev_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = nor_read_range,
	},

	.block_size = 1,
};

static io_block_spec_t nor_dev_fip_spec;

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = mtk_plat_nor_setup();
	if (ret)
		return ret;

	mtk_fip_location(&nor_dev_fip_spec.offset, &nor_dev_fip_spec.length);

	ret = register_io_dev_block(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&nor_dev_spec, dev_handle);
	if (ret)
		return ret;

	*image_spec = (uintptr_t)&nor_dev_fip_spec;

	return 0;
}
