/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _NMBM_MTD_H_
#define _NMBM_MTD_H_

#include <linux/mtd/mtd.h>

int nmbm_attach_mtd(struct mtd_info *lower, int flags, uint32_t max_ratio,
		    uint32_t max_reserved_blocks, struct mtd_info **upper);

int nmbm_free_mtd(struct mtd_info *upper);

struct mtd_info *nmbm_mtd_get_upper_by_index(uint32_t index);
struct mtd_info *nmbm_mtd_get_upper(struct mtd_info *lower);

void nmbm_mtd_list_devices(void);
int nmbm_mtd_print_info(const char *name);
int nmbm_mtd_print_states(const char *name);
int nmbm_mtd_print_bad_blocks(const char *name);
int nmbm_mtd_print_mappings(const char *name, int printall);

#endif /* _NMBM_MTD_H_ */
