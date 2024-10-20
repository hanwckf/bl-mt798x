// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <linux/mtd/mtd.h>
#include <linux/sizes.h>
#include <mtd.h>
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "autoboot_helper.h"
#include "colored_print.h"

static int write_part(const char *partname, const void *data, size_t size,
		      bool verify)
{
	struct mtd_info *mtd;
	int ret;

	gen_mtd_probe_devices();

	mtd = get_mtd_device_nm(partname);
	if (IS_ERR(mtd)) {
		cprintln(ERROR, "*** MTD partition '%s' not found! ***",
			 partname);
		put_mtd_device(mtd);
		return -PTR_ERR(mtd);
	}

	ret = mtd_update_generic(mtd, data, size, verify);

	put_mtd_device(mtd);

	return ret;
}

int write_bl(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part("bootloader", data, size, true);
}

int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return write_part("firmware", data, size, true);
}

static int write_flash_image(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	struct mtd_info *mtd;
	int ret;

	gen_mtd_probe_devices();

#ifdef CONFIG_ENABLE_NAND_NMBM
	mtd = get_mtd_device_nm("nmbm0");
#else
	mtd = get_mtd_device(NULL, 0);
#endif

	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, NULL,
				false);
	if (ret) {
		put_mtd_device(mtd);
		return ret;
	}

	ret = mtd_write_skip_bad(mtd, 0, size, mtd->size, NULL, data, true);

	put_mtd_device(mtd);

	return ret;
}

static int erase_env(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	int ret = 0;
#if !defined(CONFIG_MTK_SECURE_BOOT) && defined(CONFIG_ENV_IS_IN_MTD)
	struct mtd_info *mtd;

	gen_mtd_probe_devices();

	mtd = get_mtd_device_nm(CONFIG_ENV_MTD_NAME);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
				mtd->size, NULL, "environment", false);

	put_mtd_device(mtd);
#endif

	return ret;
}

static const struct data_part_entry snand_parts[] = {
	{
		.name = "Bootloader",
		.abbr = "bl",
		.env_name = "bootfile.bl",
		.write = write_bl,
		.post_action = UPGRADE_ACTION_CUSTOM,
		//.do_post_action = erase_env,
	},
	{
		.name = "Firmware",
		.abbr = "fw",
		.env_name = "bootfile",
		.post_action = UPGRADE_ACTION_BOOT,
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
	*dpes = snand_parts;
	*count = ARRAY_SIZE(snand_parts);
}

int board_boot_default(void)
{
	struct mtd_info *mtd;

	gen_mtd_probe_devices();

	mtd = get_mtd_device_nm("firmware");
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		return boot_from_mtd(mtd, 0);
	}

	return -ENODEV;
}

static const struct bootmenu_entry snand_bootmenu_entries[] = {
	{
		.desc = "Startup system (Default)",
		.cmd = "mtkboardboot"
	},
	{
		.desc = "Upgrade firmware",
		.cmd = "mtkupgrade fw"
	},
	{
		.desc = "Upgrade Bootloader",
		.cmd = "mtkupgrade bl"
	},
	{
		.desc = "Upgrade single image",
		.cmd = "mtkupgrade simg"
	},
	{
		.desc = "Load image",
		.cmd = "mtkload"
	}
};

void board_bootmenu_entries(const struct bootmenu_entry **menu, u32 *count)
{
	*menu = snand_bootmenu_entries;
	*count = ARRAY_SIZE(snand_bootmenu_entries);
}
