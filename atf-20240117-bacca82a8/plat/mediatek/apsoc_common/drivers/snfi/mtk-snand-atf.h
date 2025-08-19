/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_SNAND_ATF_H_
#define _MTK_SNAND_ATF_H_

#include <stddef.h>
#include <stdint.h>

struct mtk_snand;

int mtk_snand_read_range(struct mtk_snand *snf, uint64_t addr, uint64_t maxaddr,
			 void *buf, size_t len, size_t *retlen,
			 void *page_cache);

void mtk_snand_set_buf_pool(void *buf);

#endif /* _MTK_SNAND_ATF_H_ */
