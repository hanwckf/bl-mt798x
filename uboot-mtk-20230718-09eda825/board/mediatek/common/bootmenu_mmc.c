// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <command.h>
#include <linux/sizes.h>
#include <errno.h>
#include <dm/ofnode.h>
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "autoboot_helper.h"
#include "verify_helper.h"
#include "mmc_helper.h"
#include "colored_print.h"
#include "bl2_helper.h"
#include "fip_helper.h"

#define MMC_DEV_INDEX		CONFIG_MTK_BOOTMENU_MMC_DEV_INDEX

#define GPT_MAX_SIZE		(34 * 512)

#define PART_BL2_NAME		"bl2"
#define PART_FIP_NAME		"fip"

static int write_part(const char *partname, const void *data, size_t size,
		      bool verify)
{
	return mmc_write_part(MMC_DEV_INDEX, 0, partname, data, size, verify);
}

#ifdef CONFIG_MTK_FIP_SUPPORT
static int read_part(const char *partname, void *data, size_t *size,
		     size_t max_size)
{
	u64 part_size;
	int ret;

	ret = mmc_read_part_size(MMC_DEV_INDEX, 0, partname, &part_size);
	if (ret)
		return -EINVAL;

	if (part_size > (u64)max_size)
		return -ENOBUFS;

	*size = (size_t)part_size;

	return mmc_read_part(MMC_DEV_INDEX, 0, partname, data, *size);
}
#endif

static int write_bl2(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	int ret;

	if (mmc_is_sd(MMC_DEV_INDEX))
		return write_part(PART_BL2_NAME, data, size, true);

	ret = mmc_write_generic(MMC_DEV_INDEX, 1, 0, SZ_1M, data, size, true);

	if (!ret)
		mmc_setup_boot_options(MMC_DEV_INDEX);

	return ret;
}

static int write_fip(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return write_part(PART_FIP_NAME, data, size, true);
}

#ifdef CONFIG_MTK_FIP_SUPPORT
static int write_bl31(void *priv, const struct data_part_entry *dpe,
		      const void *data, size_t size)
{
	size_t fip_part_size;
	size_t new_fip_size;
	size_t buf_size;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = read_part("fip", buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	if (size > fip_part_size) {
		cprintln(ERROR, "*** Invalid image size (0x%lx) ***", size);
		return -EINVAL;
	}

	ret = fip_update_bl31_data(data, size, buf, fip_part_size,
				   &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_part("fip", buf, new_fip_size, true);
}

static int write_bl33(void *priv, const struct data_part_entry *dpe,
		      const void *data, size_t size)
{
	size_t fip_part_size;
	size_t new_fip_size;
	size_t buf_size;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = read_part("fip", buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	if (size > fip_part_size) {
		cprintln(ERROR, "*** Invalid image size (0x%lx) ***", size);
		return -EINVAL;
	}

	ret = fip_update_uboot_data(data, size, buf, fip_part_size,
				    &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_part("fip", buf, new_fip_size, true);
}
#endif

static int validate_bl2_image(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_BL2_VERIFY)) {
		ret = bl2_check_image_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** BL2 verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

static int validate_simg_image(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size)
{
	int ret = 0;

	ret = validate_bl2_image(priv, dpe, data, size);

	if (ret) {
		cprintln(ERROR, "*** Single image must include bl2 image ***");
		cprintln(ERROR, "*** Single image verification failed ***");
	}

	return ret;
}

static int validate_fip_image(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_FIP_VERIFY)) {
		ret = fip_check_uboot_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** FIP verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

#ifdef CONFIG_MTK_FIP_SUPPORT
static int validate_bl31_image(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_FIP_VERIFY)) {
		ret = check_bl31_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** U-boot verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

static int validate_bl33_image(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_FIP_VERIFY)) {
		ret = check_uboot_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** U-boot verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}
#endif

static int validate_firmware(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	struct owrt_image_info ii;
	bool ret, verify_rootfs;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_VERIFY)) {
		verify_rootfs = CONFIG_IS_ENABLED(MTK_UPGRADE_IMAGE_ROOTFS_VERIFY);

		ret = verify_image_ram(data, size, SZ_512K, verify_rootfs, &ii, NULL,
				       NULL);
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
	return mmc_upgrade_image(MMC_DEV_INDEX, data, size);
}

static int write_gpt(void *priv, const struct data_part_entry *dpe,
		     const void *data, size_t size)
{
	return mmc_write_gpt(MMC_DEV_INDEX, 0, GPT_MAX_SIZE, data, size);
}

static int write_flash_image(void *priv, const struct data_part_entry *dpe,
			     const void *data, size_t size)
{
	return mmc_write_generic(MMC_DEV_INDEX, 0, 0, 0, data, size, true);
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

	return mmc_erase_env_part(MMC_DEV_INDEX, 0, env_part_name,
				  CONFIG_ENV_SIZE);
#else
	return 0;
#endif
}

static const struct data_part_entry mmc_parts[] = {
	{
		.name = "ATF BL2",
		.abbr = "bl2",
		.env_name = "bootfile.bl2",
		.validate = validate_bl2_image,
		.write = write_bl2,
	},
	{
		.name = "ATF FIP",
		.abbr = "fip",
		.env_name = "bootfile.fip",
		.validate = validate_fip_image,
		.write = write_fip,
		.post_action = UPGRADE_ACTION_CUSTOM,
		.do_post_action = erase_env,
	},
#ifdef CONFIG_MTK_FIP_SUPPORT
	{
		.name = "BL31 of ATF FIP",
		.abbr = "bl31",
		.env_name = "bootfile.bl31",
		.validate = validate_bl31_image,
		.write = write_bl31,
		.post_action = UPGRADE_ACTION_CUSTOM,
	},
	{
		.name = "BL33 of ATF FIP",
		.abbr = "bl33",
		.env_name = "bootfile.bl33",
		.validate = validate_bl33_image,
		.write = write_bl33,
		.post_action = UPGRADE_ACTION_CUSTOM,
		.do_post_action = erase_env,
	},
#endif
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
		.validate = validate_simg_image,
	},
	{
		.name = "Partition table",
		.abbr = "gpt",
		.env_name = "bootfile.gpt",
		.write = write_gpt,
	}
};

void board_upgrade_data_parts(const struct data_part_entry **dpes, u32 *count)
{
	*dpes = mmc_parts;
	*count = ARRAY_SIZE(mmc_parts);
}

int board_boot_default(void)
{
	return mmc_boot_image(MMC_DEV_INDEX);
}

static const struct bootmenu_entry mmc_bootmenu_entries[] = {
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
	*menu = mmc_bootmenu_entries;
	*count = ARRAY_SIZE(mmc_bootmenu_entries);
}
