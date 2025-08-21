// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <u-boot/crc.h>
#include "bsp_conf.h"

struct mtk_bsp_conf_data curr_bspconf, orig_bspconf;
uint32_t curr_bspconf_index;

static bool persist_bsp_conf_available;

static void __update_bsp_conf(struct mtk_bsp_conf_data *bspconf)
{
	bspconf->ver = MTK_BSP_CONF_VER;
	bspconf->len = sizeof(*bspconf);
	bspconf->crc = crc32(0, (const uint8_t *)&bspconf->len,
			     bspconf->len - sizeof(bspconf->crc));
}

static int check_bsp_conf(const struct mtk_bsp_conf_data *bspconf,
			  uint32_t index)
{
	uint32_t crc;

	if (bspconf->len < offsetof(struct mtk_bsp_conf_data, len) +
			   sizeof(bspconf->len)) {
		printf("BSP configuration %u is invalid\n", index);
		return -EBADMSG;
	}

	if (bspconf->len > sizeof(*bspconf)) {
		printf("BSP configuration %u length (%u) unsupported\n", index,
		       bspconf->len);
		return -EBADMSG;
	}

	crc = crc32(0, (const uint8_t *)&bspconf->len,
		    bspconf->len - sizeof(bspconf->crc));
	if (crc != bspconf->crc) {
		printf("BSP configuration %u is corrupted\n", index);
		return -EBADMSG;
	}

	if (bspconf->ver > MTK_BSP_CONF_VER) {
		printf("BSP configuration %u version (%u) unsupported\n",
		       index, bspconf->ver);
		return -EBADMSG;
	}

	if (bspconf->ver == MTK_BSP_CONF_VER &&
	    bspconf->len < sizeof(*bspconf)) {
		printf("BSP configuration %u size (%u) mismatch\n",
		       index, bspconf->len);
		return -EBADMSG;
	}

	printf("BSP configuration %u is OK\n", index);

	return 0;
}

void import_bsp_conf(const struct mtk_bsp_conf_data *bspconf1,
		     const struct mtk_bsp_conf_data *bspconf2)
{
	bool bc1_ok = false, bc2_ok = false;
	const struct mtk_bsp_conf_data *bc;

	persist_bsp_conf_available = false;

	if (bspconf1 && !check_bsp_conf(bspconf1, 1))
		bc1_ok = true;

	if (bspconf2 && !check_bsp_conf(bspconf2, 2))
		bc2_ok = true;

	if (!bc1_ok && !bc2_ok) {
		printf("No usable BSP configuration\n");
		return;
	}

	persist_bsp_conf_available = true;

	if (bc1_ok && !bc2_ok) {
		bc = bspconf1;
	} else if (!bc1_ok && bc2_ok) {
		bc = bspconf2;
	} else {
		if (bspconf1->upd_cnt == UINT32_MAX && bspconf2->upd_cnt == 0)
			bc = bspconf2;
		else if (bspconf2->upd_cnt == UINT32_MAX &&
			 bspconf1->upd_cnt == 0)
			bc = bspconf1;
		else if (bspconf1->upd_cnt > bspconf2->upd_cnt)
			bc = bspconf1;
		else if (bspconf2->upd_cnt > bspconf1->upd_cnt)
			bc = bspconf2;
		else
			bc = bspconf1;
	}

	curr_bspconf_index = bc == bspconf1 ? 1 : 2;

	printf("Using BSP configuration %u\n", curr_bspconf_index);

	memcpy(&curr_bspconf, bc, sizeof(curr_bspconf));

	if (curr_bspconf.len < sizeof(curr_bspconf)) {
		memset(((uint8_t *)&curr_bspconf) + curr_bspconf.len, 0,
		       sizeof(curr_bspconf) - curr_bspconf.len);
	}

	memcpy(&orig_bspconf, &curr_bspconf, sizeof(curr_bspconf));
}

int __weak board_save_bsp_conf(const struct mtk_bsp_conf_data *bspconf,
				 uint32_t index)
{
	return -ENOTSUPP;
}

int __save_bsp_conf(bool forced)
{
	uint32_t next_index = 1;
	int ret;

	__update_bsp_conf(&curr_bspconf);

	if (curr_bspconf_index) {
		if (!forced && !memcmp(&curr_bspconf, &orig_bspconf,
				       sizeof(curr_bspconf))) {
			return 0;
		}

		if (curr_bspconf_index == 1)
			next_index = 2;
	}

	curr_bspconf.upd_cnt++;
	__update_bsp_conf(&curr_bspconf);

	ret = board_save_bsp_conf(&curr_bspconf, next_index);
	if (ret) {
		if (ret == -ENOTSUPP)
			printf("Error: board didn't provide a way to update BSP configuration\n");
		else
			printf("Error: failed to update BSP configuration %u\n",
			       next_index);

		return ret;
	}

	printf("BSP configuration %u has been updated\n", next_index);

	memcpy(&orig_bspconf, &curr_bspconf, sizeof(curr_bspconf));

	curr_bspconf_index = next_index;

	return 0;
}

void import_bsp_conf_bl2(uintptr_t top_base)
{
	struct mtk_bsp_conf_data bspconf;
	int ret;

	memcpy(&bspconf, (void *)(top_base - sizeof(bspconf)), sizeof(bspconf));

	if (check_bsp_conf(&bspconf, 0)) {
		printf("ATF BL2 did not pass correct BSP configuration\n");
		return;
	}

	if (curr_bspconf_index) {
		bspconf.upd_cnt = curr_bspconf.upd_cnt;
		__update_bsp_conf(&bspconf);

		if (!memcmp(&bspconf, &curr_bspconf, sizeof(bspconf))) {
			printf("BSP configuration passed by ATF BL2 is unchanged\n");
			return;
		}
	}

	printf("BSP configuration passed by ATF BL2 has changed\n");

	memcpy(&curr_bspconf, &bspconf, sizeof(curr_bspconf));

	ret = __save_bsp_conf(true);
	if (ret)
		return;

	if (!persist_bsp_conf_available)
		__save_bsp_conf(true);

	persist_bsp_conf_available = true;
}

int save_bsp_conf(void)
{
	return __save_bsp_conf(false);
}
