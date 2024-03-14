/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/spi_nor.h>
#include <mtk_spi.h>
#include "bl2_plat_setup.h"

int mtk_plat_nor_setup(void)
{
	unsigned long long size;
	uint32_t src_clk;
	int ret;

	src_clk = mtk_plat_get_qspi_src_clk();

	mtk_qspi_setup_buffer((void *)QSPI_BUF_OFFSET);

	ret = mtk_qspi_init(src_clk);
	if (ret) {
		ERROR("mtk_qspi_init() failed with %d\n", ret);
		return ret;
	}

	ret = spi_nor_init(&size, NULL);
	if (ret) {
		ERROR("spi_nor_init() failed with %d\n", ret);
		return ret;
	}

	return 0;
}
