// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <mtd.h>
#include <ubi_uboot.h>
#include <linux/mtd/mtd.h>
#include <linux/sizes.h>

#include "upgrade_helper.h"
#include "autoboot_helper.h"
#include "verify_helper.h"
#include "mtd_helper.h"
#include "colored_print.h"

#ifdef CONFIG_ENV_IS_IN_UBI
#if (CONFIG_ENV_UBI_VID_OFFSET == 0)
 #define UBI_VID_OFFSET NULL
#else
 #define UBI_VID_OFFSET QUOTE(CONFIG_ENV_UBI_VID_OFFSET)
#endif
#endif /* CONFIG_ENV_IS_IN_UBI */

static struct mtd_info *get_mtd_part(const char *partname)
{
	struct mtd_info *mtd;

	gen_mtd_probe_devices();

	if (partname)
		mtd = get_mtd_device_nm(partname);
	else
		mtd = get_mtd_device(NULL, 0);

	return mtd;
}

static int write_part_try_names(const char *partnames[], const void *data,
				size_t size, bool verify)
{
	struct mtd_info *mtd;
	int ret;

	while (*partnames) {
		mtd = get_mtd_part(*partnames);
		if (IS_ERR(mtd)) {
			if (PTR_ERR(mtd) == -ENODEV)
				goto next_partname;

			cprintln(ERROR, "*** Failed to get MTD partition '%s'! ***",
				 *partnames);

			return PTR_ERR(mtd);
		}

		ret = mtd_update_generic(mtd, data, size, verify);

		put_mtd_device(mtd);

		return ret;

	next_partname:
		partnames++;
	}

	cprintln(ERROR, "*** MTD partition '%s' not found! ***",
		 partnames[0]);

	return -ENODEV;
}

static int write_bl(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	static const char *bl_partnames[] = { "bootloader", "u-boot", NULL };

	return write_part_try_names(bl_partnames, data, size, true);
}

static int validate_firmware(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	struct owrt_image_info ii;
	bool ret, verify_rootfs;
	struct mtd_info *mtd;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_VERIFY)) {
		mtd = get_mtd_part(NULL);
		if (IS_ERR(mtd))
			return -PTR_ERR(mtd);

		put_mtd_device(mtd);

		verify_rootfs = IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_ROOTFS_VERIFY);

		ret = verify_image_ram(data, size, mtd->erasesize, verify_rootfs, &ii,
				       NULL, NULL);
		if (ret)
			return 0;

		cprintln(ERROR, "*** Firmware integrity verification failed ***");
		return -EBADMSG;
	}

	return 0;
}

static int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	int ret;

	ret = mtd_upgrade_image(data, size);
	if (!ret)
		return 0;

	cprintln(ERROR, "*** Image not supported! ***");
	return -ENOTSUPP;
}

static int write_flash_image(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	struct mtd_info *mtd;
	int ret;

#ifdef CONFIG_ENABLE_NAND_NMBM
	mtd = get_mtd_part("nmbm0");
#else
	mtd = get_mtd_part(NULL);
#endif

	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_update_generic(mtd, data, size, true);

	put_mtd_device(mtd);

	return ret;
}

static int erase_env(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
#if !defined(CONFIG_MTK_SECURE_BOOT) && !defined(CONFIG_ENV_IS_NOWHERE) && \
    !defined(CONFIG_MTK_DUAL_BOOT)
#ifdef CONFIG_ENV_IS_IN_UBI
	if (ubi_part(CONFIG_ENV_UBI_PART, UBI_VID_OFFSET))
		return -EIO;

	ubi_remove_vol(CONFIG_ENV_UBI_VOLUME);
	ubi_exit();
#else
	struct mtd_info *mtd;
	int ret;

	mtd = get_mtd_part(CONFIG_ENV_MTD_NAME);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
				 mtd->size, NULL, "environment", false);

	put_mtd_device(mtd);

	return ret;
#endif
#else
	return 0;
#endif
}

static const struct data_part_entry mtd_parts[] = {
	{
		.name = "Bootloader",
		.abbr = "bl",
		.env_name = "bootfile.bl",
		.write = write_bl,
		.post_action = UPGRADE_ACTION_CUSTOM,
		.do_post_action = erase_env,
	},
	{
		.name = "Firmware",
		.abbr = "fw",
		.env_name = "bootfile",
		.post_action = UPGRADE_ACTION_BOOT,
		.validate = validate_firmware,
		.write = write_firmware,
	},
	{
		.name = "Single image",
		.abbr = "simg",
		.env_name = "bootfile.simg",
		.write = write_flash_image,
	},
};

void board_upgrade_data_parts(const struct data_part_entry **dpes, u32 *count)
{
	*dpes = mtd_parts;
	*count = ARRAY_SIZE(mtd_parts);
}

int board_boot_default(void)
{
	return mtd_boot_image();
}

static const struct bootmenu_entry mtd_bootmenu_entries[] = {
	{
		.desc = "Startup system (Default)",
		.cmd = "mtkboardboot"
	},
	{
		.desc = "Upgrade firmware",
		.cmd = "mtkupgrade fw"
	},
	{
		.desc = "Upgrade bootloader",
		.cmd = "mtkupgrade bl"
	},
	{
		.desc = "Upgrade single image",
		.cmd = "mtkupgrade simg"
	},
	{
		.desc = "Load image",
		.cmd = "mtkload"
	},
#ifdef CONFIG_MTK_WEB_FAILSAFE
	{
		.desc = "Start Web failsafe",
		.cmd = "httpd"
	},
#endif
};

void board_bootmenu_entries(const struct bootmenu_entry **menu, u32 *count)
{
	*menu = mtd_bootmenu_entries;
	*count = ARRAY_SIZE(mtd_bootmenu_entries);
}
