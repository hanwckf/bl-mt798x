// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <inttypes.h>
#include <common/debug.h>
#include <drivers/nand.h>
#include <mtk-snand.h>
#include <mtk-snand-atf.h>
#include "bl2_plat_setup.h"

static struct mtk_snand *snf;

static int snfi_mtd_block_is_bad(unsigned int block)
{
	struct nand_device *nand_dev = get_nand_device();
	uint64_t addr = (uint64_t)block * nand_dev->block_size;

	return mtk_snand_block_isbad(snf, addr);
}

static int snfi_mtd_read_page(struct nand_device *nand, unsigned int page,
			       uintptr_t buffer)
{
	uint64_t addr = (uint64_t)page * nand->page_size;
	int ret;

	ret = mtk_snand_read_page(snf, addr, (void *)buffer, NULL, false);
	if (ret > 0) {
		NOTICE("corrected %d bitflips while reading page %u\n", ret, page);
		ret = 0;
	}

	return ret;
}

int mtk_plat_nand_setup(size_t *page_size, size_t *block_size, uint64_t *size)
{
	struct nand_device *nand_dev = get_nand_device();
	struct mtk_snand_chip_info cinfo;
	int ret;

	mtk_snand_set_buf_pool((void *)QSPI_BUF_OFFSET);

	ret = mtk_snand_init(NULL, mtk_plat_get_snfi_platdata(), &snf);
	if (ret) {
		ERROR("mtk_snand_init() failed with %d\n", ret);
		snf = NULL;
		return ret;
	}

	mtk_snand_get_chip_info(snf, &cinfo);

	NOTICE("SPI-NAND: %s (%" PRIu64 "MB)\n", cinfo.model,
	       cinfo.chipsize >> 20);

	nand_dev->mtd_block_is_bad = snfi_mtd_block_is_bad;
	nand_dev->mtd_read_page = snfi_mtd_read_page;
	nand_dev->nb_planes = 1;
	nand_dev->block_size = cinfo.blocksize;
	nand_dev->page_size = cinfo.pagesize;
	nand_dev->oob_size = cinfo.sparesize;
	nand_dev->size = cinfo.chipsize;
	nand_dev->buswidth = 1;

	if (page_size)
		*page_size = cinfo.pagesize;

	if (block_size)
		*block_size = cinfo.blocksize;

	if (size)
		*size = cinfo.chipsize;

	return ret;
}
