// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <inttypes.h>
#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_ubi.h>
#include <drivers/nand.h>
#include "bl2_plat_setup.h"
#ifdef DUAL_FIP
#include "bsp_conf.h"
#include "dual_fip.h"
#endif

#ifdef OVERRIDE_UBI_START_ADDR
#define UBI_START_ADDR			OVERRIDE_UBI_START_ADDR
#ifdef OVERRIDE_UBI_END_ADDR
#define UBI_END_ADDR			OVERRIDE_UBI_END_ADDR
#else
#define UBI_END_ADDR			0
#endif
#else
#define UBI_START_ADDR			0x200000
#define UBI_END_ADDR			0
#endif

static uintptr_t ubi_dev_handle;

static int nand_ubispl_is_bad_block(uint32_t pnum)
{
	struct nand_device *nand_dev = get_nand_device();
	int ret;

	ret = nand_dev->mtd_block_is_bad(pnum);
	if (ret)
		return -EIO;

	return 0;
}

static int nand_ubispl_read(uint32_t pnum, unsigned long offset,
			    unsigned long len, void *dst)
{
	struct nand_device *nand_dev = get_nand_device();
	size_t len_read;
	uint64_t addr;
	int ret;

	addr = (uint64_t)pnum * nand_dev->block_size + offset;

	ret = nand_read(addr, (uintptr_t)dst, len, &len_read);
	if (ret < 0) {
		ERROR("nand_read(%" PRIu64 ") failed with %d. %zu bytes read\n",
		      addr, ret, len_read);
		return ret;
	}

	return 0;
}

static io_ubi_dev_spec_t nand_ubi_dev_spec = {
	.ubi = (struct ubi_scan_info *)SCRATCH_BUF_OFFSET,
	.is_bad_peb = nand_ubispl_is_bad_block,
	.read = nand_ubispl_read,
	.fastmap = 0,
};

static const io_ubi_spec_t ubi_dev_fip_spec = {
	.vol_id = -1,
	.vol_name = "fip",
};

#ifdef DUAL_FIP
static const io_ubi_spec_t ubi_dev_fip2_spec = {
	.vol_id = -1,
	.vol_name = "fip2",
};

static const uintptr_t ubi_dev_fips[] = {
	(uintptr_t)&ubi_dev_fip_spec,
	(uintptr_t)&ubi_dev_fip2_spec,
};

static io_ubi_spec_t ubi_dev_fip_curr_spec;

static const io_ubi_spec_t ubi_dev_bootconf1_spec = {
	.vol_id = -1,
	.vol_name = MTK_BSP_CONF_NAME "1",
};

static const io_ubi_spec_t ubi_dev_bootconf2_spec = {
	.vol_id = -1,
	.vol_name = MTK_BSP_CONF_NAME "2",
};

static void mtk_load_bsp_conf_ubi(uintptr_t dev_handle)
{
	struct mtk_bsp_conf_data bc1, bc2, *pbc1 = NULL, *pbc2 = NULL;
	int ret;

	ret = mtk_dev_read_spec(dev_handle, (uintptr_t)&ubi_dev_bootconf1_spec,
				&bc1, 0, sizeof(bc1),
				ubi_dev_bootconf1_spec.vol_name);
	if (!ret)
		pbc1 = &bc1;

	ret = mtk_dev_read_spec(dev_handle, (uintptr_t)&ubi_dev_bootconf2_spec,
				&bc2, 0, sizeof(bc2),
				ubi_dev_bootconf2_spec.vol_name);
	if (!ret)
		pbc2 = &bc2;

	import_bsp_conf(pbc1, pbc2);
}
#endif

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	size_t page_size, block_size;
	uintptr_t fip_image_spec;
	uint64_t nand_size;
	int ret;

#ifdef DUAL_FIP
	uintptr_t dev_handles[FIP_NUM];
	uint32_t slot;
#endif

	ret = mtk_plat_nand_setup(&page_size, &block_size, &nand_size);
	if (ret)
		return ret;

	nand_ubi_dev_spec.peb_size = block_size;
	nand_ubi_dev_spec.peb_offset = UBI_START_ADDR / block_size;
	nand_ubi_dev_spec.peb_count = ((UBI_END_ADDR ?: nand_size) - UBI_START_ADDR) /
				      block_size;
	nand_ubi_dev_spec.vid_offset = page_size;
	nand_ubi_dev_spec.leb_start = page_size * 2;

	ret = register_io_dev_ubi(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)&nand_ubi_dev_spec,
			  &ubi_dev_handle);
	if (ret)
		return ret;

	fip_image_spec = (uintptr_t)&ubi_dev_fip_spec;

#ifdef DUAL_FIP
	mtk_load_bsp_conf_ubi(ubi_dev_handle);

	dev_handles[0] = dev_handles[1] = ubi_dev_handle;

	ret = dual_fip_check(dev_handles, ubi_dev_fips, &slot);
	if (ret)
		return ret;

	ubi_dev_fip_curr_spec = *(io_ubi_spec_t *)ubi_dev_fips[slot];
	fip_image_spec = (uintptr_t)&ubi_dev_fip_curr_spec;
#endif

	*dev_handle = ubi_dev_handle;
	*image_spec = fip_image_spec;

	return 0;
}

#ifdef DUAL_FIP
int mtk_fip_image_setup_next_slot(void)
{
	uintptr_t dev_handles[FIP_NUM];
	uint32_t slot;
	int ret;

	dev_handles[0] = dev_handles[1] = ubi_dev_handle;

	ret = dual_fip_next_slot(dev_handles, ubi_dev_fips, &slot);
	if (ret)
		return ret;

	ubi_dev_fip_curr_spec = *(io_ubi_spec_t *)ubi_dev_fips[slot];

	return 0;
}
#endif
