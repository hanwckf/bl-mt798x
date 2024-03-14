/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <mtk_spi_nand.h>
#include <mtk_spi.h>
#include "bl2_plat_setup.h"

int mtk_plat_nand_setup(size_t *page_size, size_t *block_size, uint64_t *size)
{
	struct nand_device *nand_dev;
	unsigned long long chip_size;
	unsigned int erase_size;
	uint32_t src_clk;
	int ret;

	src_clk = mtk_plat_get_qspi_src_clk();

	mtk_qspi_setup_buffer((void *)QSPI_BUF_OFFSET);

	ret = mtk_qspi_init(src_clk);
	if (ret) {
		ERROR("mtk_qspi_init() failed with %d\n", ret);
		return ret;
	}

	ret = spi_nand_init(&chip_size, &erase_size);
	if (ret) {
		ERROR("spi_nand_init() failed with %d\n", ret);
		return ret;
	}

	nand_dev = get_nand_device();

	if (page_size)
		*page_size = nand_dev->page_size;

	if (block_size)
		*block_size = erase_size;

	if (size)
		*size = chip_size;

	return 0;
}
