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

#include <mempool.h>
#include <drivers/spi_nor.h>
#include <mtk_spi.h>
#include <boot_spi.h>

#define FIP_BASE_NOR	0x100000
#define FIP_SIZE_NOR	0x80000

static size_t spim_nor_read(int lba, uintptr_t buf, size_t size)
{
/* less than buffer size allocated by SPI module */
#define READ_LENGTH_ONCE	0x1000
	size_t length_read = 0;
	size_t len_read_current = 0;
	size_t ret_size = size;
	int ret = 0;

	while (size) {
		len_read_current = (size > READ_LENGTH_ONCE ? READ_LENGTH_ONCE : size);
		ret = spi_nor_read(lba, buf, len_read_current, &length_read);
		if (ret < 0) {
			ERROR("spi nor read fail: %d, read length: %ld\n", ret, length_read);
			return 0;
		}

		buf += len_read_current;
		lba += len_read_current;
		size -= len_read_current;
	}

	return ret_size;
}

static size_t spim_nor_write(int lba, uintptr_t buf, size_t size)
{
	/* Do not use write in BL2 */
	return 0;
}

static io_block_dev_spec_t spim_nor_dev_spec = {
	/* Buffer should not exceed BL33_BASE */
	.buffer = {
		.offset = 0x41000000,
		.length = 0xe00000,
	},

	.ops = {
		.read = spim_nor_read,
		.write = spim_nor_write,
	},

	.block_size = 0x1
};

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset = FIP_BASE_NOR,
	.length = FIP_SIZE_NOR,
};

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	unsigned long long size;
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

	result = spi_nor_init(&size, NULL);
	if (result) {
		ERROR("spi nor init fail %d\n", result);
		assert(result == 0);
	}

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&spim_nor_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}


