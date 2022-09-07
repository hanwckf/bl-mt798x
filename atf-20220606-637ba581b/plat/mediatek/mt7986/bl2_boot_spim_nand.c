/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/delay_timer.h>
#include <drivers/io/io_driver.h>
#include <lib/utils_def.h>
#include <bl2_boot_dev.h>
#include <mt7986_def.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <lib/mmio.h>
#include <string.h>
#include <lib/utils.h>

#include <drivers/nand.h>
#include <drivers/spi_nand.h>
#include <mempool.h>
#include <mtk_spi.h>
#include <boot_spi.h>

#ifdef NMBM
#include <nmbm/nmbm.h>
#endif

#define FIP_BASE			0x380000
#define FIP_SIZE			0x200000

#ifndef NMBM_MAX_RATIO
#define NMBM_MAX_RATIO			1
#endif

#ifndef NMBM_MAX_RESERVED_BLOCKS
#define NMBM_MAX_RESERVED_BLOCKS	256
#endif

#ifdef NMBM
static struct nmbm_instance *ni;

static int nmbm_lower_read_page(void *arg, uint64_t addr, void *buf, void *oob,
				enum nmbm_oob_mode mode)
{
	struct nand_device *nand_dev;

	nand_dev = get_nand_device();
	if (!nand_dev) {
		ERROR("Failed to get spi-nand device\n");
		return -EINVAL;
	}

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
	struct nand_device *nand_dev;

	nand_dev = get_nand_device();
	if (!nand_dev) {
		ERROR("Failed to get spi-nand device\n");
		return -EINVAL;
	}

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
	struct nand_device *nand_dev;
	struct nmbm_lower_device nld;
	size_t ni_size;
	int ret;

	nand_dev = get_nand_device();
	if (!nand_dev) {
		ERROR("Failed to get spi-nand device\n");
		return -EINVAL;
	}

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
	ni = mtk_mem_pool_alloc(ni_size);
	memset(ni, 0, ni_size);

	NOTICE("Initializing NMBM ...\n");

	ret = nmbm_attach(&nld, ni);
	if (ret) {
		ni = NULL;
		return ret;
	}

	return 0;
}

static size_t spim_nand_read_range(int lba, uintptr_t buf, size_t size)
{
	struct nand_device *nand_dev;
	size_t retlen = 0;

	nand_dev = get_nand_device();
	if (!nand_dev) {
		ERROR("Failed to get spi-nand device\n");
		return -EINVAL;
	}

	nmbm_read_range(ni, lba * nand_dev->page_size, size, (void *)buf,
			NMBM_MODE_PLACE_OOB, &retlen);

	return retlen;
}
#else
static size_t spim_nand_read_range(int lba, uintptr_t buf, size_t size)
{
	struct nand_device *nand_dev;
	size_t length_read;
	uint64_t off;
	int ret = 0;

	nand_dev = get_nand_device();
	if (nand_dev == NULL) {
		ERROR("spinand get device fail\n");
		return -EINVAL;
	}

	off = lba * nand_dev->page_size;
	ret = nand_read(off, buf, size, &length_read);
	if (ret < 0)
		ERROR("spinand read fail: %d, read length: %ld\n", ret, length_read);

	return length_read;
}
#endif

static size_t spim_nand_write_range(int lba, uintptr_t buf, size_t size)
{
	/* Do not use write in BL2 */
	return 0;
}

static io_block_dev_spec_t spim_nand_dev_spec = {
	/* Buffer should not exceed BL33_BASE */
	.buffer = {
		.offset = 0x41000000,
		.length = 0xe00000,
	},

	.ops = {
		.read = spim_nand_read_range,
		.write = spim_nand_write_range,
	},
};

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset = FIP_BASE,
	.length = FIP_SIZE,
};

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	struct nand_device *nand_dev;
	unsigned long long size;
	unsigned int erase_size;
	int result;

	/* config GPIO pinmux to spi mode */
	mtk_spi_gpio_init();

	/* select 208M clk */
	mtk_spi_source_clock_select(CLK_MPLL_D2);

	result = mtk_qspi_init(CLK_MPLL_D2);
	if (result) {
		ERROR("mtk spi init fail %d\n", result);
		assert(result == 0);
	}

	result = spi_nand_init(&size, &erase_size);
	if (result) {
		ERROR("spi nand init fail %d\n", result);
		assert(result == 0);
	}

	nand_dev = get_nand_device();
	assert(nand_dev == NULL);

	spim_nand_dev_spec.block_size = nand_dev->page_size;

#ifdef NMBM
	result = nmbm_init();
	assert(result == 0);
#endif

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&spim_nand_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
