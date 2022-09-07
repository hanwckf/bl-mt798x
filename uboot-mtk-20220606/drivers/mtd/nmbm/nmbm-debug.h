/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Debug addons for NAND Mapped-block Management (NMBM)
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _NMBM_DEBUG_H_
#define _NMBM_DEBUG_H_

#include "nmbm-private.h"

#define nmbm_mark_block_color_normal(ni, start_ba, end_ba)
#define nmbm_mark_block_color_bad(ni, ba)
#define nmbm_mark_block_color_mgmt(ni, start_ba, end_ba)
#define nmbm_mark_block_color_signature(ni, ba)
#define nmbm_mark_block_color_info_table(ni, start_ba, end_ba)
#define nmbm_mark_block_color_mapped(ni, ba)

uint32_t nmbm_debug_get_block_state(struct nmbm_instance *ni, uint32_t ba);
char nmbm_debug_get_phys_block_type(struct nmbm_instance *ni, uint32_t ba);

enum nmmb_block_type {
	NMBM_BLOCK_GOOD_DATA,
	NMBM_BLOCK_GOOD_MGMT,
	NMBM_BLOCK_BAD,
	NMBM_BLOCK_MAIN_INFO_TABLE,
	NMBM_BLOCK_BACKUP_INFO_TABLE,
	NMBM_BLOCK_REMAPPED,
	NMBM_BLOCK_SIGNATURE,

	__NMBM_BLOCK_TYPE_MAX
};

#endif /* _NMBM_DEBUG_H_ */
