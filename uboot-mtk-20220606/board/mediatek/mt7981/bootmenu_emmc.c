// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <asm/global_data.h>
#include <command.h>
#include <fdtdec.h>
#include <linux/sizes.h>
#include <errno.h>
#include <dm/ofnode.h>
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "autoboot_helper.h"
#include "verify_helper.h"
#include "colored_print.h"
#include "untar.h"

DECLARE_GLOBAL_DATA_PTR;

/*
 * Keep this value be up-to-date with definition in libfstools/rootdisk.c of
 * fstools package
 */
#define ROOTDEV_OVERLAY_ALIGN	(64ULL * 1024ULL)

#define EMMC_DEV_INDEX		(0)
#define BOOTBUS_WIDTH		(2)
#define RESET_BOOTBUS_WIDTH	(0)
#define BOOT_MODE		(0)
#define BOOT_ACK		(1)
#define BOOT_PARTITION		(1)
#define PARTITION_ACCESS	(0)
#define RESET_VALUE		(1) /* only 0, 1, 2 are valid */

#define GPT_MAX_SIZE		(34 * 512)

#define PART_FIP_NAME		"fip"
#define PART_KERNEL_NAME	"kernel"
#define PART_ROOTFS_NAME	"rootfs"

#ifdef CONFIG_MTK_DUAL_BOOT
#include "dual_boot.h"

struct mmc_boot_slot dual_boot_slots[DUAL_BOOT_MAX_SLOTS] = {
	{
		.kernel = PART_KERNEL_NAME,
		.rootfs = PART_ROOTFS_NAME,
	},
	{
		.kernel = CONFIG_MTK_DUAL_BOOT_SLOT_1_KERNEL_NAME,
		.rootfs = CONFIG_MTK_DUAL_BOOT_SLOT_1_ROOTFS_NAME,
	}
};
#endif /* CONFIG_MTK_DUAL_BOOT */

static int write_part(const char *part, const void *data, size_t size,
		      bool verify)
{
	return mmc_write_part(EMMC_DEV_INDEX, 0, part, data, size, verify);
}

static int erase_part(const char *part, loff_t offset, u64 size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	u64 part_size;
	int ret;

	mmc = _mmc_get_dev(EMMC_DEV_INDEX, 0, false);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part, &dpart);
	if (ret)
		return ret;

	part_size = (u64)dpart.size * dpart.blksz;

	if (offset >= part_size)
		return -EINVAL;

	if (!size || offset + size > part_size)
		size = part_size - offset;

	return _mmc_erase(mmc, (u64)dpart.start * dpart.blksz + offset, 0,
			  size);
}

int write_bl2(void *priv, const struct data_part_entry *dpe,
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

int write_fip(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part(PART_FIP_NAME, data, size, true);
}

#ifdef CONFIG_MTK_UPGRADE_IMAGE_VERIFY
static int validate_firmware(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	struct owrt_image_info ii;
	bool ret, verify_rootfs;

	verify_rootfs = CONFIG_IS_ENABLED(MTK_UPGRADE_IMAGE_ROOTFS_VERIFY);

	ret = verify_image_ram(data, size, SZ_512K, verify_rootfs, &ii, NULL,
			       NULL);
	if (ret)
		return 0;

	cprintln(ERROR, "*** Firmware integrity verification failed ***");
	return -EBADMSG;
}
#endif /* CONFIG_MTK_UPGRADE_IMAGE_VERIFY */

int write_firmware(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	const char *kernel_part, *rootfs_part;
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	loff_t rootfs_data_offs;
	int ret = 0;
#ifdef CONFIG_MTK_DUAL_BOOT
	u32 slot;
#endif /* CONFIG_MTK_DUAL_BOOT */

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

#ifdef CONFIG_MTK_DUAL_BOOT
	slot = dual_boot_get_next_slot();
	printf("Upgrading image slot %u ...\n", slot);

	kernel_part = dual_boot_slots[slot].kernel;
	rootfs_part = dual_boot_slots[slot].rootfs;
#else
	kernel_part = PART_KERNEL_NAME;
	rootfs_part = PART_ROOTFS_NAME;
#endif /* CONFIG_MTK_DUAL_BOOT */

	ret = write_part(rootfs_part, rootfs_data, rootfs_size, true);
	if (ret)
		return ret;

	/* Mark rootfs_data unavailable */
	rootfs_data_offs = (rootfs_size + ROOTDEV_OVERLAY_ALIGN - 1) &
			   (~(ROOTDEV_OVERLAY_ALIGN - 1));
	erase_part(PART_ROOTFS_NAME, rootfs_data_offs, SZ_512K);

	ret = write_part(kernel_part, kernel_data, kernel_size, true);
	if (ret)
		return ret;

#ifdef CONFIG_MTK_DUAL_BOOT
	ret = dual_boot_set_current_slot(slot);
	if (ret)
		printf("Error: failed to save new image slot to env\n");
#endif /* CONFIG_MTK_DUAL_BOOT */

	return ret;
}

int write_gpt(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return mmc_write_gpt(EMMC_DEV_INDEX, 0, GPT_MAX_SIZE, data, size);
}

static int write_flash_image(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	return mmc_write_generic(EMMC_DEV_INDEX, 0, 0, 0, data, size, true);
}

static int erase_env(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
#if !defined(CONFIG_MTK_SECURE_BOOT) && !defined(CONFIG_ENV_IS_NOWHERE) && \
    !defined(CONFIG_MTK_DUAL_BOOT)
	const char *env_part_name =
		ofnode_conf_read_str("u-boot,mmc-env-partition");

	if (!env_part_name)
		return 0;

	return mmc_erase_env_part(EMMC_DEV_INDEX, 0, env_part_name,
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
		//.do_post_action = erase_env,
	},
	{
		.name = "Firmware",
		.abbr = "fw",
		.env_name = "bootfile",
		.post_action = UPGRADE_ACTION_BOOT,
#ifdef CONFIG_MTK_UPGRADE_IMAGE_VERIFY
		.validate = validate_firmware,
#endif /* CONFIG_MTK_UPGRADE_IMAGE_VERIFY */
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
#ifdef CONFIG_MTK_DUAL_BOOT
	struct mmc_boot_data mbd = { 0 };

	mbd.dev = EMMC_DEV_INDEX;
	mbd.boot_slots = dual_boot_slots;
	mbd.env_part = ofnode_conf_read_str("u-boot,mmc-env-partition");
	mbd.load_ptr = (void *)0x40000000;
#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
	mbd.rootfs_data_part = CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME;
#endif

	return dual_boot_mmc(&mbd);
#else
	return boot_from_mmc_partition(EMMC_DEV_INDEX, 0, PART_KERNEL_NAME);
#endif /* CONFIG_MTK_DUAL_BOOT */
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
