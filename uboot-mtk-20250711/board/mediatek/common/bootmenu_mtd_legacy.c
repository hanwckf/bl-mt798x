// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include "bootmenu_common.h"
#include "autoboot_helper.h"
#include "mtd_helper.h"
#include "colored_print.h"

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

static const struct data_part_entry mtd_parts[] = {
	{
		.name = "Bootloader",
		.abbr = "bl",
		.env_name = "bootfile.bl",
		.write = write_bl,
		.post_action = UPGRADE_ACTION_CUSTOM,
		//.do_post_action = generic_invalidate_env,
	},
	{
		.name = "Firmware",
		.abbr = "fw",
		.env_name = "bootfile",
		.post_action = UPGRADE_ACTION_BOOT,
		.validate = generic_mtd_validate_fw,
		.write = generic_mtd_write_fw,
	},
	{
		.name = "Single image",
		.abbr = "simg",
		.env_name = "bootfile.simg",
		.write = generic_mtd_write_simg,
	},
};

void board_upgrade_data_parts(const struct data_part_entry **dpes, u32 *count)
{
	*dpes = mtd_parts;
	*count = ARRAY_SIZE(mtd_parts);
}

int board_boot_default(bool do_boot)
{
	return generic_mtd_boot_image(do_boot);
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
	{
		.desc = "Change boot configuration",
		.cmd = "mtkbootconf"
	},
};

void board_bootmenu_entries(const struct bootmenu_entry **menu, u32 *count)
{
	*menu = mtd_bootmenu_entries;
	*count = ARRAY_SIZE(mtd_bootmenu_entries);
}

int board_late_init(void)
{
	return 0;
}
