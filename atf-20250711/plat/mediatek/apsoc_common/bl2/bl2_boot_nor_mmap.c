// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <drivers/io/io_driver.h>
#include <drivers/io/io_memmap.h>
#include "bl2_plat_setup.h"

#define NOR_MAP_BASE		0x30000000

static io_block_spec_t nor_dev_fip_spec;

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	mtk_fip_location(&nor_dev_fip_spec.offset, &nor_dev_fip_spec.length);

	nor_dev_fip_spec.offset += NOR_MAP_BASE;

	ret = register_io_dev_memmap(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)NULL, dev_handle);
	if (ret)
		return ret;

	*image_spec = (uintptr_t)&nor_dev_fip_spec;

	return 0;
}
