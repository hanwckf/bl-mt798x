// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <common.h>
#include <config.h>
#include <env.h>
#include <init.h>
#include <asm/global_data.h>
#include <asm/io.h>
#include <linux/libfdt.h>

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

#define	MT7986_BOOT_NOR		0
#define	MT7986_BOOT_SPIM_NAND	1
#define	MT7986_BOOT_EMMC	2
#define	MT7986_BOOT_SNFI_NAND	3

int ft_system_setup(void *blob, struct bd_info *bd)
{
	const u32 *media_handle_p;
	int chosen, len, ret;
	const char *media;
	u32 media_handle;

	switch ((readl(0x1001f6f0) & 0x300) >> 8) {
	case MT7986_BOOT_NOR:
		media = "rootdisk-nor";
		break
		;;
	case MT7986_BOOT_SPIM_NAND:
		media = "rootdisk-spim-nand";
		break
		;;
	case MT7986_BOOT_EMMC:
		media = "rootdisk-emmc";
		break
		;;
	case MT7986_BOOT_SNFI_NAND:
		media = "rootdisk-sd";
		break
		;;
	}

	chosen = fdt_path_offset(blob, "/chosen");
	if (chosen <= 0)
		return 0;

	media_handle_p = fdt_getprop(blob, chosen, media, &len);
	if (media_handle_p <= 0 || len != 4)
		return 0;

	media_handle = *media_handle_p;
	ret = fdt_setprop(blob, chosen, "rootdisk", &media_handle, sizeof(media_handle));
	if (ret) {
		printf("cannot set media phandle %s as rootdisk /chosen node\n", media);
		return ret;
	}

	printf("set /chosen/rootdisk to bootrom media: %s (phandle 0x%08x)\n", media, fdt32_to_cpu(media_handle));

	return 0;
}
