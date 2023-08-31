// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * NMBM initialization routine
 */

#include <mtd.h>
#include <linux/mtd/mtd.h>

#include <nmbm/nmbm.h>
#include <nmbm/nmbm-mtd.h>

int board_nmbm_init(void)
{
	struct mtd_info *lower, *upper;
	int ret;

	printf("\n");
	printf("Initializing NMBM ...\n");

	mtd_probe_devices();

	lower = get_mtd_device_nm(CONFIG_NMBM_LOWER_MTD);
	if (IS_ERR(lower) || !lower) {
		printf("Lower MTD device 'spi-nand0' not found\n");
		return 0;
	}

	ret = nmbm_attach_mtd(lower,
			      NMBM_F_CREATE | NMBM_F_EMPTY_PAGE_ECC_OK,
			      CONFIG_NMBM_MAX_RATIO,
			      CONFIG_NMBM_MAX_BLOCKS, &upper);

	printf("\n");

	if (ret)
		return 0;

	add_mtd_device(upper);

	return 0;
}
