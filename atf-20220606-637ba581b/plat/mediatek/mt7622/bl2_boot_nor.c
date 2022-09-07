/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_memmap.h>
#include <bl2_boot_dev.h>

#define MT7622_NOR_MAP_BASE		0x30000000

#define FIP_BASE			0x20000
#define FIP_SIZE			0x80000

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset	= MT7622_NOR_MAP_BASE + FIP_BASE,
	.length = FIP_SIZE,
};

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	int result;

	result = register_io_dev_memmap(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)NULL, boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
