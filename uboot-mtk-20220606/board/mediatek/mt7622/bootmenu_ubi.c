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
#include "ubi_helper.h"
#include "autoboot_helper.h"
#include "colored_print.h"

static int write_part(const char *partname, const void *data, size_t size,
		      bool verify)
{
	struct mtd_info *mtd;
	int ret;

	ubi_probe_mtd_devices();

	mtd = get_mtd_device_nm(partname);
	if (IS_ERR(mtd)) {
		cprintln(ERROR, "*** MTD partition '%s' not found! ***",
			 partname);
		return -PTR_ERR(mtd);
	}

	put_mtd_device(mtd);

	ret = mtd_erase_generic(mtd, 0, size);
	if (ret)
		return ret;

	return mtd_write_generic(mtd, 0, 0, data, size, verify);
}

static int write_bl2(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part("bl2", data, size, true);
}

static int write_fip(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part("fip", data, size, true);
}

static int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	int ret;

	ret = ubi_upgrade_image(data, size);
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

	ubi_probe_mtd_devices();

#ifdef CONFIG_ENABLE_NAND_NMBM
	mtd = get_mtd_device_nm("nmbm0");
#else
	mtd = get_mtd_device(NULL, 0);
#endif

	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	put_mtd_device(mtd);

	ret = mtd_erase_generic(mtd, 0, size);
	if (ret)
		return ret;

	return mtd_write_generic(mtd, 0, 0, data, size, true);
}

static int erase_env(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
#if !defined(CONFIG_ENV_IS_NOWHERE)
	struct mtd_info *mtd;

	ubi_probe_mtd_devices();

	mtd = get_mtd_device_nm(CONFIG_ENV_MTD_NAME);

	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	put_mtd_device(mtd);

	return mtd_erase_env(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE);
#else
	return 0;
#endif
}

static const struct data_part_entry snand_parts[] = {
	{
		.name = "ATF BL2",
		.abbr = "bl2",
		.env_name = "bootfile.bl2",
		.write = write_bl2,
	},
	{
		.name = "ATF FIP",
		.abbr = "fip",
		.env_name = "bootfile.fip",
		.write = write_fip,
		.post_action = UPGRADE_ACTION_CUSTOM,
		.do_post_action = erase_env,
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
	return ubi_boot_image();
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
		.desc = "Upgrade ATF BL2",
		.cmd = "mtkupgrade bl2"
	},
	{
		.desc = "Upgrade ATF FIP",
		.cmd = "mtkupgrade fip"
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
