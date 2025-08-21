// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include "bootmenu_common.h"
#include "autoboot_helper.h"
#include "mmc_helper.h"

static const struct data_part_entry snor_emmc_parts[] = {
	{
		.name = "ATF BL2",
		.abbr = "bl2",
		.env_name = "bootfile.bl2",
		.validate = generic_validate_bl2,
		.write = generic_mtd_write_bl2,
	},
	{
		.name = "ATF FIP",
		.abbr = "fip",
		.env_name = "bootfile.fip",
		.validate = generic_validate_fip,
#if defined(CONFIG_FIP_IN_SPI_NOR)
		.write = generic_mtd_write_fip,
#elif defined(CONFIG_FIP_IN_EMMC)
		.write = generic_mmc_write_fip,
#endif
		.post_action = UPGRADE_ACTION_CUSTOM,
		//.do_post_action = generic_invalidate_env,
	},
#ifdef CONFIG_MTK_FIP_SUPPORT
	{
		.name = "BL31 of ATF FIP",
		.abbr = "bl31",
		.env_name = "bootfile.bl31",
		.validate = generic_validate_bl31,
#if defined(CONFIG_FIP_IN_SPI_NOR)
		.write = generic_mtd_update_bl31,
#elif defined(CONFIG_FIP_IN_EMMC)
		.write = generic_mmc_update_bl31,
#endif
		.post_action = UPGRADE_ACTION_CUSTOM,
	},
	{
		.name = "BL33 of ATF FIP",
		.abbr = "bl33",
		.env_name = "bootfile.bl33",
		.validate = generic_validate_bl33,
#if defined(CONFIG_FIP_IN_SPI_NOR)
		.write = generic_mtd_update_bl33,
#elif defined(CONFIG_FIP_IN_EMMC)
		.write = generic_mmc_update_bl33,
#endif
		.post_action = UPGRADE_ACTION_CUSTOM,
		//.do_post_action = generic_invalidate_env,
	},
#endif
	{
		.name = "Firmware",
		.abbr = "fw",
		.env_name = "bootfile",
		.post_action = UPGRADE_ACTION_BOOT,
		.validate = generic_mmc_validate_fw,
		.write = generic_mmc_write_fw,
	},
	{
		.name = "Single image (SPI-NOR)",
		.abbr = "simg-snor",
		.env_name = "bootfile.simg-snor",
		.write = generic_mtd_write_simg,
	},
	{
		.name = "Single image (eMMC)",
		.abbr = "simg-emmc",
		.env_name = "bootfile.simg-emmc",
		.write = generic_mmc_write_simg,
	},
	{
		.name = "Partition table",
		.abbr = "gpt",
		.env_name = "bootfile.gpt",
		.write = generic_mmc_write_gpt,
	}
};

void board_upgrade_data_parts(const struct data_part_entry **dpes, u32 *count)
{
	*dpes = snor_emmc_parts;
	*count = ARRAY_SIZE(snor_emmc_parts);
}

int board_boot_default(bool do_boot)
{
	return generic_mmc_boot_image(do_boot);
}

static const struct bootmenu_entry snor_emmc_bootmenu_entries[] = {
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
#ifdef CONFIG_MTK_FIP_SUPPORT
	{
		.desc = "  Upgrade ATF BL31 only",
		.cmd = "mtkupgrade bl31"
	},
	{
		.desc = "  Upgrade bootloader only",
		.cmd = "mtkupgrade bl33"
	},
#endif
	{
		.desc = "Upgrade partition table",
		.cmd = "mtkupgrade gpt"
	},
	{
		.desc = "Upgrade single image (SPI-NOR)",
		.cmd = "mtkupgrade simg-snor"
	},
	{
		.desc = "Upgrade single image (eMMC)",
		.cmd = "mtkupgrade simg-emmc"
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
	*menu = snor_emmc_bootmenu_entries;
	*count = ARRAY_SIZE(snor_emmc_bootmenu_entries);
}

void default_boot_set_defaults(void *fdt)
{
	mmc_boot_set_defaults(fdt);
}
