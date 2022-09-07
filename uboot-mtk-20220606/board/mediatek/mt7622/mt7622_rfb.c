// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2018 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <common.h>
#include <config.h>
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <mtd.h>
#include <linux/mtd/mtd.h>

#include <nmbm/nmbm.h>
#include <nmbm/nmbm-mtd.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;
	return 0;
}

int board_late_init(void)
{
	gd->env_valid = 1; //to load environment variable from persistent store
	env_relocate();
	return 0;
}

int board_nmbm_init(void)
{
#ifdef CONFIG_ENABLE_NAND_NMBM
	struct mtd_info *lower, *upper;
	int ret;

	printf("\n");
	printf("Initializing NMBM ...\n");

	mtd_probe_devices();

	lower = get_mtd_device_nm("spi-nand0");
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
#endif

	return 0;
}
