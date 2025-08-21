/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_BSP_CONF_H_
#define _MTK_BSP_CONF_H_

#include <stddef.h>
#include <stdint.h>

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

int mtk_io_read_incr(uintptr_t image_handle, void *buff, size_t len);
int mtk_dev_read_spec(uintptr_t dev_handle, uintptr_t spec, void *buff,
		      uint64_t offs, size_t len, const char *spec_name);

void import_bsp_conf(const struct mtk_bsp_conf_data *bspconf1,
		     const struct mtk_bsp_conf_data *bspconf2);
void update_bsp_conf(void);
void finalize_bsp_conf(uintptr_t top_base);

#endif /* _MTK_BSP_CONF_H_ */
