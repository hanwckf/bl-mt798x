// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <inttypes.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_block.h>
#include <drivers/nand.h>
#include <nmbm/nmbm.h>
#include "bl2_plat_setup.h"

#ifndef NMBM_MAX_RATIO
#define NMBM_MAX_RATIO 1
#endif

#ifndef NMBM_MAX_RESERVED_BLOCKS
#define NMBM_MAX_RESERVED_BLOCKS 256
#endif

static struct nmbm_instance *ni = (struct nmbm_instance *)SCRATCH_BUF_OFFSET;

static int nmbm_lower_read_page(void *arg, uint64_t addr, void *buf, void *oob,
				enum nmbm_oob_mode mode)
{
	struct nand_device *nand_dev = get_nand_device();

	/*
	 * Currently reading oob region is not supported by spi-nand driver,
	 * and it's also not used by NMBM in read-only mode.
	 * So just fill oob with full 0xff data.
	 */
	if (oob)
		memset(oob, 0xff, nand_dev->oob_size);

	return nand_dev->mtd_read_page(nand_dev, addr / nand_dev->page_size,
				       (uintptr_t)buf);
}

static int nmbm_lower_is_bad_block(void *arg, uint64_t addr)
{
	struct nand_device *nand_dev = get_nand_device();

	return nand_dev->mtd_block_is_bad(addr / nand_dev->block_size);
}

static void nmbm_lower_log(void *arg, enum nmbm_log_category level,
			   const char *fmt, va_list ap)
{
	int log_level;
	const char *prefix_str;

	switch (level) {
	case NMBM_LOG_DEBUG:
		log_level = LOG_LEVEL_VERBOSE;
		break;
	case NMBM_LOG_WARN:
		log_level = LOG_LEVEL_WARNING;
		break;
	case NMBM_LOG_ERR:
	case NMBM_LOG_EMERG:
		log_level = LOG_LEVEL_ERROR;
		break;
	default:
		log_level = LOG_LEVEL_NOTICE;
	}

	if (log_level > LOG_LEVEL)
		return;

	prefix_str = plat_log_get_prefix(log_level);

	while (*prefix_str != '\0') {
		(void)putchar(*prefix_str);
		prefix_str++;
	}

	vprintf(fmt, ap);
}

static int nmbm_init(void)
{
	struct nand_device *nand_dev = get_nand_device();
	struct nmbm_lower_device nld;
	size_t ni_size;
	int ret;

	memset(&nld, 0, sizeof(nld));

	nld.flags = NMBM_F_CREATE | NMBM_F_READ_ONLY;
	nld.max_ratio = NMBM_MAX_RATIO;
	nld.max_reserved_blocks = NMBM_MAX_RESERVED_BLOCKS;

	nld.size = nand_dev->size;
	nld.erasesize = nand_dev->block_size;
	nld.writesize = nand_dev->page_size;
	nld.oobsize = nand_dev->oob_size;

	nld.read_page = nmbm_lower_read_page;
	nld.is_bad_block = nmbm_lower_is_bad_block;

	nld.logprint = nmbm_lower_log;

	ni_size = nmbm_calc_structure_size(&nld);
	memset(ni, 0, ni_size);

	NOTICE("Initializing NMBM ...\n");

	ret = nmbm_attach(&nld, ni);
	if (ret) {
		ni = NULL;
		return ret;
	}

	return 0;
}

static size_t nmbm_read(int lba, uintptr_t buf, size_t size)
{
	struct nand_device *nand_dev = get_nand_device();
	size_t retlen = 0;

	nmbm_read_range(ni, (uint64_t)lba * nand_dev->page_size, size,
			(void *)buf, NMBM_MODE_PLACE_OOB, &retlen);

	return retlen;
}

static io_block_dev_spec_t nand_dev_spec = {
	.buffer = {
		.offset = IO_BLOCK_BUF_OFFSET,
		.length = IO_BLOCK_BUF_SIZE,
	},

	.ops = {
		.read = nmbm_read,
	},
};

static io_block_spec_t nand_dev_fip_spec;

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	ret = mtk_plat_nand_setup(&nand_dev_spec.block_size, NULL, NULL);
	if (ret)
		return ret;

	mtk_fip_location(&nand_dev_fip_spec.offset, &nand_dev_fip_spec.length);

	ret = nmbm_init();
	if (ret)
		return ret;

	ret = register_io_dev_block(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&nand_dev_spec, dev_handle);
	if (ret)
		return ret;

	*image_spec = (uintptr_t)&nand_dev_fip_spec;

	return 0;
}
