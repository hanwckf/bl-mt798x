// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <init.h>
#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/sizes.h>
#include <linux/types.h>
#include <linux/log2.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

#define	MT7987_BOOT_SD		0
#define	MT7987_BOOT_NOR		1
#define	MT7987_BOOT_SPIM_NAND	2
#define	MT7987_BOOT_EMMC	3

const char *mtk_board_rootdisk(void)
{
	switch ((readl(0x1001f6f0) & 0xc0) >> 6) {
	case MT7987_BOOT_SD:
		return "sd";

	case MT7987_BOOT_NOR:
		return "nor";

	case MT7987_BOOT_SPIM_NAND:
		return "spim-nand";

	case MT7987_BOOT_EMMC:
		return "emmc";

	default:
		return "";
	}
}

ulong board_get_load_addr(void)
{
	ulong half_size = (get_effective_memsize() / 2) & ~(SZ_16M - 1);

	return gd->ram_base + half_size;
}
