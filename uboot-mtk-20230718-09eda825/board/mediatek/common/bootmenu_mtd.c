// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
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
#include "bl2_helper.h"
#include "fip_helper.h"

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

	if (IS_ERR(mtd)) {
		cprintln(ERROR, "*** MTD partition '%s' not found! ***",
			 partname);
	}

	return mtd;
}

#ifdef CONFIG_MTK_FIP_SUPPORT
static int read_part(const char *partname, void *data, size_t *size,
		     size_t max_size)
{
	struct mtd_info *mtd;
	u64 part_size;
	int ret;

	mtd = get_mtd_part(partname);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	part_size = mtd->size;

	if (part_size > (u64)max_size) {
		ret = -ENOBUFS;
		goto err;
	}

	*size = (size_t)part_size;

	ret = mtd_read_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, data);

err:
	put_mtd_device(mtd);

	return ret;
}
#endif

static int write_part(const char *partname, const void *data, size_t size,
		      bool verify)
{
	struct mtd_info *mtd;
	int ret;

	mtd = get_mtd_part(partname);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_update_generic(mtd, data, size, verify);

	put_mtd_device(mtd);

	return ret;
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
			cprintln(ERROR, "*** BL31 verification failed ***");
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

#if !defined(CONFIG_MTK_SECURE_BOOT) && !defined(CONFIG_ENV_IS_NOWHERE) && \
    !defined(CONFIG_MTK_DUAL_BOOT)
#ifdef CONFIG_ENV_IS_IN_UBI
	if (ubi_part(CONFIG_ENV_UBI_PART, UBI_VID_OFFSET))
		return -EIO;

	ubi_remove_vol(CONFIG_ENV_UBI_VOLUME);
	ubi_exit();
#else
	struct mtd_info *mtd;

	mtd = get_mtd_part(CONFIG_ENV_MTD_NAME);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
				 mtd->size, NULL, "environment", false);

	put_mtd_device(mtd);
#endif
#endif

	return ret;
}

static const struct data_part_entry mtd_parts[] = {
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
		.validate = validate_simg_image,
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
