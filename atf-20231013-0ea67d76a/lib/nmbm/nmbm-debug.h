/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
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

static void nmbm_mark_block_color_normal(struct nmbm_instance *ni,
					 uint32_t start_ba, uint32_t end_ba) {}
static void nmbm_mark_block_color_bad(struct nmbm_instance *ni, uint32_t ba) {}
static void nmbm_mark_block_color_mgmt(struct nmbm_instance *ni,
				       uint32_t start_ba, uint32_t end_ba) {}
static void nmbm_mark_block_color_signature(struct nmbm_instance *ni,
					    uint32_t ba) {}
static void nmbm_mark_block_color_info_table(struct nmbm_instance *ni,
					     uint32_t start_ba,
					     uint32_t end_ba) {}
static void nmbm_mark_block_color_mapped(struct nmbm_instance *ni, uint32_t ba) {}

#endif /* _NMBM_DEBUG_H_ */
