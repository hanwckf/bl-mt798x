// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <errno.h>
#include <linux/sizes.h>
#include <errno.h>
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "autoboot_helper.h"
#include "colored_print.h"
#include "untar.h"

#define ROOTDEV_OVERLAY_ALIGN   (64ULL * 1024ULL)

#define SD_DEV_INDEX		0

enum upgrade_part_type {
	UPGRADE_PART_GPT,
	UPGRADE_PART_BL2,
	UPGRADE_PART_FIP,
	UPGRADE_PART_KERNEL,
	UPGRADE_PART_ROOTFS,
};

struct upgrade_part {
	const char *name;
	u64 offset;
	size_t size;
};

static const struct upgrade_part upgrade_parts[] = {
	[UPGRADE_PART_GPT] = {
		.offset = 0,
		.size = 0x4400,
	},
	[UPGRADE_PART_BL2] = {
		.name = "bl2",
		.offset = 0x4400,
		.size = 0x3FBC00,
	},

	[UPGRADE_PART_FIP] = {
		.name = "fip",
		.offset = 0x680000,
		.size = 0x200000,
	},
	[UPGRADE_PART_KERNEL] = {
		.name = "kernel",
		.offset = 0x880000,
		.size = 0x2000000,
	},
	[UPGRADE_PART_ROOTFS] = {
		.name = "rootfs",
		.offset = 0x2880000,
		.size = 0x10000000,
	},
};

static int write_part(enum upgrade_part_type pt, const void *data, size_t size,
		      bool verify)
{
	const struct upgrade_part *part = &upgrade_parts[pt];
	int ret;

	if (part->name) {
		ret = mmc_write_part(SD_DEV_INDEX, 0, part->name, data, size,
				     verify);
		if (ret != -ENODEV)
			return ret;

		cprintln(CAUTION, "*** Fallback to fixed offset ***");
	}

	return mmc_write_generic(SD_DEV_INDEX, 0, part->offset, part->size,
				 data, size, verify);
}


static int erase_part(enum upgrade_part_type pt, loff_t offset, u64 size)
{
	const struct upgrade_part *part = &upgrade_parts[pt];
	struct disk_partition dpart;
	struct mmc *mmc;
	u64 part_size;
	int ret;

	if (part->name) {
		mmc = _mmc_get_dev(SD_DEV_INDEX, 0, false);
		if (!mmc)
			return -ENODEV;

		ret = _mmc_find_part(mmc, part->name, &dpart);
		if (ret)
			goto fixed_erase;

		part_size = (u64)dpart.size * dpart.blksz;

		if (offset >= part_size)
			return -EINVAL;

		if (!size || offset + size > part_size)
			size = part_size - offset;

		ret = _mmc_erase(mmc, (u64)dpart.start * dpart.blksz + offset,
				 0, size);
		if (!ret)
			return ret;

		cprintln(CAUTION, "*** Fallback to fixed offset ***");
	}

fixed_erase:
	if (!size)
		size = part->size;

	return mmc_erase_generic(SD_DEV_INDEX, 0, part->offset + offset,
				 size);
}

static int write_bl2(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part(UPGRADE_PART_BL2, data, size, true);
}

static int write_fip(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part(UPGRADE_PART_FIP, data, size, true);
}

static int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	loff_t rootfs_data_offs;
	int ret = 0, ret2 = 0;

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	ret = write_part(UPGRADE_PART_KERNEL, kernel_data, kernel_size, true);
	ret2 = write_part(UPGRADE_PART_ROOTFS, rootfs_data, rootfs_size, true);

	/* Mark rootfs_data unavailable */
	rootfs_data_offs = (rootfs_size + ROOTDEV_OVERLAY_ALIGN - 1) &
			   (~(ROOTDEV_OVERLAY_ALIGN - 1));
	erase_part(UPGRADE_PART_ROOTFS, rootfs_data_offs, SZ_512K);

	return ret ? ret : ret2;
}

static int write_gpt(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	const struct upgrade_part *part = &upgrade_parts[UPGRADE_PART_GPT];

	return mmc_write_gpt(SD_DEV_INDEX, 0, part->size, data, size);
}

static int write_flash_image(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	return mmc_write_generic(SD_DEV_INDEX, 0, 0, 0, data, size, true);
}

static int erase_env(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
#if !defined(CONFIG_ENV_IS_NOWHERE)
	return mmc_erase_env_generic(SD_DEV_INDEX, 0, CONFIG_ENV_OFFSET,
				     CONFIG_ENV_SIZE);
#else
	return 0;
#endif
}

static const struct data_part_entry sd_parts[] = {
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
		.name = "SD Partition table",
		.abbr = "gpt",
		.env_name = "bootfile.gpt",
		.write = write_gpt,
	}
};

void board_upgrade_data_parts(const struct data_part_entry **dpes, u32 *count)
{
	*dpes = sd_parts;
	*count = ARRAY_SIZE(sd_parts);
}

int board_boot_default(void)
{
	const struct upgrade_part *part = &upgrade_parts[UPGRADE_PART_KERNEL];

	boot_from_mmc_partition(SD_DEV_INDEX, 0, part->name);

	return boot_from_mmc_generic(SD_DEV_INDEX, 0, part->offset);
}

static const struct bootmenu_entry sd_bootmenu_entries[] = {
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
		.desc = "Upgrade SD partition table",
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
	*menu = sd_bootmenu_entries;
	*count = ARRAY_SIZE(sd_bootmenu_entries);
}
