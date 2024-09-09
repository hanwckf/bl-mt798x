// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <asm/io.h>
#include <linux/libfdt.h>

int board_init(void)
{
	return 0;
}

#define	MT7981_BOOT_NOR		0
#define	MT7981_BOOT_SPIM_NAND	1 /* ToDo: fallback to SD */
#define	MT7981_BOOT_EMMC	2
#define	MT7981_BOOT_SNFI_NAND	3 /* ToDo (treated as SD) */

int ft_system_setup(void *blob, struct bd_info *bd)
{
	const u32 *media_handle_p;
	int chosen, len, ret;
	const char *media;
	u32 media_handle;

	switch ((readl(0x11d006f0) & 0xc0) >> 6) {
	case MT7981_BOOT_NOR:
		media = "rootdisk-nor";
		break
		;;
	case MT7981_BOOT_SPIM_NAND:
		media = "rootdisk-spim-nand";
		break
		;;
	case MT7981_BOOT_EMMC:
		media = "rootdisk-emmc";
		break
		;;
	case MT7981_BOOT_SNFI_NAND:
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
