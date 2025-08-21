// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include "mtk-snand-def.h"

static void *buf_pool;

int mtk_snand_read_range(struct mtk_snand *snf, uint64_t addr, uint64_t maxaddr,
			 void *buf, size_t len, size_t *retlen,
			 void *page_cache)
{
	uint32_t chunksize, col;
	size_t sizeremain = len;
	bool checkbad = true;
	int ret;

	if (!snf || !buf) {
		if (retlen)
			*retlen = 0;

		return -EINVAL;
	}

	if (maxaddr > snf->size)
		maxaddr = snf->size;

	while (sizeremain && addr < maxaddr) {
		if (checkbad || !(addr & snf->erasesize_mask)) {
			ret = mtk_snand_block_isbad(snf, addr);
			if (ret) {
				mtk_snand_log(snf->pdev, SNAND_LOG_CHIP,
					      "Skipped bad block at 0x%llx\n",
					      addr);
				addr += snf->erasesize;
				checkbad = true;
				continue;
			}

			checkbad = false;
		}

		col = addr & snf->writesize_mask;

		chunksize = snf->writesize - col;
		if (chunksize > sizeremain)
			chunksize = sizeremain;

		if (addr + chunksize > maxaddr)
			chunksize = maxaddr - addr;

		ret = mtk_snand_read_page(snf, addr, page_cache, NULL, false);
		if (ret < 0) {
			if (retlen)
				*retlen = len - sizeremain;

			return ret;
		}

		memcpy(buf, page_cache + col, chunksize);

		addr += chunksize;
		buf += chunksize;
		sizeremain -= chunksize;
	}

	if (retlen)
		*retlen = len - sizeremain;

	return 0;
}

void mtk_snand_set_buf_pool(void *buf)
{
	buf_pool = buf;
}

void *mtk_snand_mem_alloc(size_t size)
{
	void *buf = buf_pool;

	if (!buf_pool)
		return NULL;

	buf_pool += size;

	return buf;
}

