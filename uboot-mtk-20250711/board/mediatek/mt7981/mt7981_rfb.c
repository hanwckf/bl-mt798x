// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/sizes.h>
#include <linux/types.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

#define	MT7981_BOOT_NOR		0
#define	MT7981_BOOT_SPIM_NAND	1
#define	MT7981_BOOT_EMMC	2
#define	MT7981_BOOT_SNFI_NAND	3

const char *mtk_board_rootdisk(void)
{
	switch ((readl(0x11d006f0) & 0xc0) >> 6) {
	case MT7981_BOOT_NOR:
		return "nor";

	case MT7981_BOOT_SPIM_NAND:
		return "spim-nand";

	case MT7981_BOOT_EMMC:
		return "emmc";

	case MT7981_BOOT_SNFI_NAND:
		return "sd";

	default:
		return "";
	}
}

ulong board_get_load_addr(void)
{
	if (gd->ram_size <= SZ_128M)
		return gd->ram_base;

	if (gd->ram_size <= SZ_256M)
		return gd->ram_top - SZ_64M;

	return gd->ram_base + SZ_256M;
}
