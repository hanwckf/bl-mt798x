/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_BSP_CONF_H_
#define _MTK_BSP_CONF_H_

#include <linux/types.h>

#define MTK_BSP_CONF_VER			1

#define MTK_BSP_CONF_NAME			"bspconf"

#define FIP_NUM					2
#define IMAGE_NUM				2

struct mtk_image_info {
	uint32_t invalid;
	uint32_t size;
	uint32_t crc32;
	uint32_t upd_cnt;
};

struct mtk_bsp_conf_data {
	uint32_t crc;
	uint32_t len;
	uint32_t ver;
	uint32_t upd_cnt;

	/* Dual FIP */
	uint32_t current_fip_slot;
	struct mtk_image_info fip[FIP_NUM];

	/* Dual image */
	uint32_t current_image_slot;
	struct mtk_image_info image[IMAGE_NUM];
};

extern struct mtk_bsp_conf_data curr_bspconf;
extern uint32_t curr_bspconf_index;

void import_bsp_conf(const struct mtk_bsp_conf_data *bspconf1,
		     const struct mtk_bsp_conf_data *bspconf2);
void import_bsp_conf_bl2(uintptr_t top_base);
int __save_bsp_conf(bool forced);
int save_bsp_conf(void);

#endif /* _MTK_BSP_CONF_H_ */
