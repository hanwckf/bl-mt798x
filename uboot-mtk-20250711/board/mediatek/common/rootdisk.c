// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Daniel Golle <daniel@makrotopia.org>
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt rootdisk settings
 */

#include <asm/u-boot.h>
#include <linux/errno.h>
#include <linux/libfdt.h>
#include "rootdisk.h"

static char media[64];

__weak const char *mtk_board_rootdisk(void)
{
	return NULL;
}

#if IS_ENABLED(CONFIG_MTK_TZ_MOVABLE)
int mtk_ft_system_setup(void *blob, struct bd_info *bd)
#else
int ft_system_setup(void *blob, struct bd_info *bd)
#endif
{
	const char *raw_media = "";
	const u32 *media_handle_p;
	int chosen, len, ret;
	u32 media_handle;

#ifdef CONFIG_MTK_ROOTDISK_OVERRIDE
	raw_media = CONFIG_MTK_ROOTDISK_OVERRIDE;
#endif
	if (!raw_media[0]) {
		raw_media = mtk_board_rootdisk();
		if (!raw_media)
			return 0;

		if (!raw_media[0]) {
			return -ENODEV;
		}
	}

	snprintf(media, sizeof(media), "rootdisk-%s", raw_media);

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

static int rootdisk_set_rootfs_ubi(void *fdt, const char *volname, bool relax)
{
	int len, ret, chosen, volume_offset;
	const u32 *media_handle_p;
	u32 media_handle;

	chosen = fdt_path_offset(fdt, "/chosen");
	if (chosen <= 0)
		return 0;

	media_handle_p = fdt_getprop(fdt, chosen, "rootdisk-spim-nand", &len);
	if (media_handle_p <= 0 || len != 4) {
		if (!relax)
			panic("'rootdisk-spim-nand' not found in image FDT blob\n");
		return -1;
	}

	media_handle = fdt32_to_cpu(*media_handle_p);

	volume_offset = fdt_node_offset_by_phandle(fdt, media_handle);
	if (volume_offset < 0) {
		if (!relax)
			panic("Failed to locate node pointed by 'rootdisk-spim-nand'\n");
		return -1;
	}

	len = strlen(volname) + 1;

	ret = fdt_setprop(fdt, volume_offset, "volname", volname, len);
	if (ret < 0) {
		if (!relax) {
			panic("Error: failed to set ubi rootfs volname to '%s'\n",
			      volname);
		}

		return -1;
	}

	printf("FIT volume for fitblk set to '%s'\n", volname);

	return 0;
}

int rootdisk_set_rootfs_ubi_relax(void *fdt, const char *volname)
{
	return rootdisk_set_rootfs_ubi(fdt, volname, true);
}

static int rootdisk_set_rootfs_block_part(void *fdt, const char *rootdisk,
					  const char *part, bool relax)
{
	int len, ret, chosen, part_offset;
	const u32 *media_handle_p;
	u32 media_handle;

	chosen = fdt_path_offset(fdt, "/chosen");
	if (chosen <= 0)
		return 0;

	media_handle_p = fdt_getprop(fdt, chosen, rootdisk, &len);
	if (media_handle_p <= 0 || len != 4) {
		if (!relax)
			panic("'%s' not found in image FDT blob\n", rootdisk);
		return -1;
	}

	media_handle = fdt32_to_cpu(*media_handle_p);

	part_offset = fdt_node_offset_by_phandle(fdt, media_handle);
	if (part_offset < 0) {
		if (!relax) {
			panic("Failed to locate node pointed by '%s'\n",
			      rootdisk);
		}
		return -1;
	}

	len = strlen(part) + 1;

	ret = fdt_setprop(fdt, part_offset, "partname", part, len);
	if (ret < 0) {
		if (!relax) {
			panic("Error: failed to set block rootfs part to '%s'\n",
			      part);
		}
		return -1;
	}

	printf("FIT part for fitblk set to '%s'\n", part);

	return 0;
}

int rootdisk_set_rootfs_block_part_relax(void *fdt, const char *part,
					 bool is_sd)
{
	return rootdisk_set_rootfs_block_part(fdt,
			is_sd ? "rootdisk-sd" : "rootdisk-emmc", part, true);
}

int rootdisk_set_fitblk_rootfs(void *fdt, const char *block_part)
{
	if (!strcmp(media, "rootdisk-spim-nand"))
		return rootdisk_set_rootfs_ubi(fdt, block_part, false);

	if (!strcmp(media, "rootdisk-emmc") || !strcmp(media, "rootdisk-sd")) {
		return rootdisk_set_rootfs_block_part(fdt, media, block_part,
						      false);
	}

	return 0;
}
