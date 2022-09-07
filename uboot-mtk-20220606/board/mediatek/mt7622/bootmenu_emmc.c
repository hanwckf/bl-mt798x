// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <command.h>
#include <linux/sizes.h>
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "autoboot_helper.h"
#include "colored_print.h"

#define EMMC_DEV_INDEX		(0)
#define BOOTBUS_WIDTH		(2)
#define RESET_BOOTBUS_WIDTH	(0)
#define BOOT_MODE		(0)
#define BOOT_ACK		(1)
#define BOOT_PARTITION		(1)
#define PARTITION_ACCESS	(0)
#define RESET_VALUE		(1) /* only 0, 1, 2 are valid */

enum upgrade_part_type {
	UPGRADE_PART_GPT,
	UPGRADE_PART_FIP,
	UPGRADE_PART_FW,
};

struct upgrade_part {
	const char *name;
	u64 offset;
	size_t size;
};

static const struct upgrade_part upgrade_parts[] = {
	[UPGRADE_PART_GPT] = {
		.offset = 0,
		.size = 0x20000,
	},
	[UPGRADE_PART_FIP] = {
		.name = "fip",
		.offset = 0x100000,
		.size = 0x200000,
	},
	[UPGRADE_PART_FW] = {
		.name = "firmware",
		.offset = 0x400000,
		.size = 0x1400000,
	},
};

static int write_part(enum upgrade_part_type pt, const void *data, size_t size,
		      bool verify)
{
	const struct upgrade_part *part = &upgrade_parts[pt];
	int ret;

	if (part->name) {
		ret = mmc_write_part(EMMC_DEV_INDEX, 0, part->name, data, size,
				     verify);
		if (ret != -ENODEV)
			return ret;

		cprintln(CAUTION, "*** Fallback to fixed offset ***");
	}

	return mmc_write_generic(EMMC_DEV_INDEX, 0, part->offset, part->size,
				 data, size, verify);
}

static int write_bl2(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	char cmd[64];
	int ret;

	ret = mmc_write_generic(EMMC_DEV_INDEX, 1, 0, SZ_1M, data, size, true);

	snprintf(cmd, sizeof(cmd), "mmc bootbus %x %x %x %x", EMMC_DEV_INDEX, BOOTBUS_WIDTH,
		 RESET_BOOTBUS_WIDTH, BOOT_MODE);
	run_command(cmd, 0);
	snprintf(cmd, sizeof(cmd), "mmc partconf %x %x %x %x", EMMC_DEV_INDEX, BOOT_ACK,
		 BOOT_PARTITION, PARTITION_ACCESS);
	run_command(cmd, 0);
	snprintf(cmd, sizeof(cmd), "mmc rst-function %x %x", EMMC_DEV_INDEX, RESET_VALUE);
	run_command(cmd, 0);

	return ret;
}

static int write_fip(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part(UPGRADE_PART_FIP, data, size, true);
}

static int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return write_part(UPGRADE_PART_FW, data, size, true);
}

static int write_gpt(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	const struct upgrade_part *part = &upgrade_parts[UPGRADE_PART_GPT];

	return mmc_write_gpt(EMMC_DEV_INDEX, 0, part->size, data, size);
}

static int write_flash_image(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	return mmc_write_generic(EMMC_DEV_INDEX, 0, 0, 0, data, size, true);
}

static int erase_env(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
#if !defined(CONFIG_ENV_IS_NOWHERE)
	return mmc_erase_env_generic(EMMC_DEV_INDEX, 0, CONFIG_ENV_OFFSET,
				     CONFIG_ENV_SIZE);
#else
	return 0;
#endif
}

static const struct data_part_entry emmc_parts[] = {
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
	{
		.name = "eMMC Partition table",
		.abbr = "gpt",
		.env_name = "bootfile.gpt",
		.write = write_gpt,
	}
};

void board_upgrade_data_parts(const struct data_part_entry **dpes, u32 *count)
{
	*dpes = emmc_parts;
	*count = ARRAY_SIZE(emmc_parts);
}

int board_boot_default(void)
{
	const struct upgrade_part *part = &upgrade_parts[UPGRADE_PART_FW];

	boot_from_mmc_partition(EMMC_DEV_INDEX, 0, part->name);

	return boot_from_mmc_generic(EMMC_DEV_INDEX, 0, part->offset);
}

static const struct bootmenu_entry emmc_bootmenu_entries[] = {
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
		.desc = "Upgrade eMMC partition table",
		.cmd = "mtkupgrade gpt"
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
	*menu = emmc_bootmenu_entries;
	*count = ARRAY_SIZE(emmc_bootmenu_entries);
}
