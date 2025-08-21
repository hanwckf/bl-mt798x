// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <string.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <common/tf_crc32.h>
#include <drivers/io/io_driver.h>
#include "bsp_conf.h"

struct mtk_bsp_conf_data curr_bspconf;

int mtk_io_read_incr(uintptr_t image_handle, void *buff, size_t len)
{
	size_t retlen, read_len = 0;
	int ret;

	while (read_len < len) {
		retlen = 0;

		ret = io_read(image_handle, (uintptr_t)buff, len - read_len,
			      &retlen);
		if (ret)
			return ret;

		if (!retlen)
			return -EIO;

		read_len += retlen;
		buff += retlen;
	}

	return  0;
}

int mtk_dev_read_spec(uintptr_t dev_handle, uintptr_t spec, void *buff,
		      uint64_t offs, size_t len, const char *spec_name)
{
	uintptr_t image_handle;
	int ret;

	ret = io_open(dev_handle, spec, &image_handle);
	if (ret) {
		ERROR("Failed to open '%s' for read (%d)\n", spec_name, ret);
		return ret;
	}

	if (offs)  {
		ret = io_seek(image_handle, IO_SEEK_SET, offs);
		if (ret) {
			ERROR("Failed to seek to 0x%lx of '%s' (%d)\n", offs,
			spec_name, ret);
			return ret;
		}
	}

	ret = mtk_io_read_incr(image_handle, buff, len);
	if (ret) {
		ERROR("Failed to read '%s' with length 0x%zx (%d)\n",
		      spec_name, len, ret);
	}

	io_close(image_handle);

	return ret;
}

static int check_bsp_conf(const struct mtk_bsp_conf_data *bspconf,
			  uint32_t index)
{
	uint32_t crc;

	if (bspconf->len < offsetof(struct mtk_bsp_conf_data, len) +
			   sizeof(bspconf->len)) {
		ERROR("BSP configuration %u is invalid\n", index);
		return -EBADMSG;
	}

	if (bspconf->len > sizeof(*bspconf)) {
		ERROR("BSP configuration %u length (%u) unsupported\n", index,
		      bspconf->len);
		return -EBADMSG;
	}

	crc = tf_crc32(0, (const uint8_t *)&bspconf->len,
		       bspconf->len - sizeof(bspconf->crc));
	if (crc != bspconf->crc) {
		ERROR("BSP configuration %u is corrupted\n", index);
		return -EBADMSG;
	}

	if (bspconf->ver > MTK_BSP_CONF_VER) {
		ERROR("BSP configuration %u version (%u) unsupported\n",
		      index, bspconf->ver);
		return -EBADMSG;
	}

	if (bspconf->ver == MTK_BSP_CONF_VER &&
	    bspconf->len < sizeof(*bspconf)) {
		ERROR("BSP configuration %u size (%u) mismatch\n",
		      index, bspconf->len);
		return -EBADMSG;
	}

	NOTICE("BSP configuration %u is OK\n", index);

	return 0;
}

void import_bsp_conf(const struct mtk_bsp_conf_data *bspconf1,
		     const struct mtk_bsp_conf_data *bspconf2)
{
	bool bc1_ok = false, bc2_ok = false;
	const struct mtk_bsp_conf_data *bc;

	if (bspconf1 && !check_bsp_conf(bspconf1, 1))
		bc1_ok = true;

	if (bspconf2 && !check_bsp_conf(bspconf2, 2))
		bc2_ok = true;

	if (!bc1_ok && !bc2_ok) {
		ERROR("No usable BSP configuration\n");
		return;
	}

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

	NOTICE("Using BSP configuration %u\n", bc == bspconf1 ? 1 : 2);

	memcpy(&curr_bspconf, bc, sizeof(curr_bspconf));

	if (curr_bspconf.len < sizeof(curr_bspconf)) {
		memset(((uint8_t *)&curr_bspconf) + curr_bspconf.len, 0,
		       sizeof(curr_bspconf) - curr_bspconf.len);
	}
}

void update_bsp_conf(void)
{
	curr_bspconf.ver = MTK_BSP_CONF_VER;
	curr_bspconf.len = sizeof(curr_bspconf);
	curr_bspconf.crc = tf_crc32(0, (const uint8_t *)&curr_bspconf.len,
				     curr_bspconf.len - sizeof(curr_bspconf.crc));
}

void finalize_bsp_conf(uintptr_t top_base)
{
	uintptr_t base = top_base - sizeof(curr_bspconf);

	update_bsp_conf();

	memcpy((void *)base, &curr_bspconf, sizeof(curr_bspconf));

	flush_dcache_range((uintptr_t)base, sizeof(curr_bspconf));
}
