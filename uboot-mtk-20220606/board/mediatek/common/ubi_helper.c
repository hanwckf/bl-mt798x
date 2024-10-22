// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt UBI image upgrading & booting helper
 */

#include <errno.h>
#include <linux/mtd/mtd.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/kernel.h>
#include <memalign.h>
#include <env.h>
#include <mtd.h>
#include <ubi_uboot.h>

#include "image_helper.h"
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "colored_print.h"
#include "untar.h"

static int write_ubi1_image(const void *data, size_t size,
			    struct mtd_info *mtd_kernel,
			    struct mtd_info *mtd_ubi,
			    struct owrt_image_info *ii)
{
	int ret;

	if (mtd_kernel->size != ii->kernel_size + ii->padding_size) {
		cprintln(ERROR, "*** Image kernel size mismatch ***");
		return -ENOTSUPP;
	}

	/* Write kernel part to kernel MTD partition */
	ret = mtd_update_generic(mtd_kernel, data, mtd_kernel->size, true);
	if (ret)
		return ret;

	/* Write ubi part to kernel MTD partition */
	return mtd_update_generic(mtd_ubi, data + mtd_kernel->size,
				  ii->ubi_size + ii->marker_size, true);
}

static int mount_ubi(struct mtd_info *mtd)
{
	struct ubi_device *ubi;
	int ret;

	ubi_exit();

	ret = ubi_mtd_param_parse(mtd->name, NULL);
	if (ret)
		return -ret;

	ret = ubi_init();
	if (ret) {
		cprintln(CAUTION, "*** Failed to attach UBI ***");
		cprintln(NORMAL, "*** Rebuilding UBI ***");

		ubi_exit();

		ubi_mtd_param_parse(mtd->name, NULL);

		ret = mtd_erase_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, NULL,
					 false);
		if (ret)
			return ret;

		ret = ubi_init();
		if (ret) {
			cprintln(ERROR, "*** Failed to attach UBI ***");
			return -ret;
		}
	}

	ubi = ubi_devices[0];

	if (ubi->ro_mode) {
		cprintln(ERROR, "*** UBI is read-only mode ***");
		ubi_exit();
		return -EROFS;
	}

	return 0;
}

static void umount_ubi(void)
{
	ubi_exit();
}

static struct ubi_volume *find_ubi_volume(const char *volume)
{
	struct ubi_volume *vol = NULL;
	struct ubi_device *ubi;
	int i;

	ubi = ubi_devices[0];

	for (i = 0; i < ubi->vtbl_slots; i++) {
		vol = ubi->volumes[i];
		if (vol && !strcmp(vol->name, volume))
			return vol;
	}

	return NULL;
}

static int remove_ubi_volume(const char *volume)
{
	int ret, reserved_pebs, i;
	struct ubi_volume *vol;
	struct ubi_device *ubi;

	ubi = ubi_devices[0];

	vol = find_ubi_volume(volume);
	if (!vol)
		return 0;

	ret = ubi_change_vtbl_record(ubi, vol->vol_id, NULL);
	if (ret)
		goto out_err;

	reserved_pebs = vol->reserved_pebs;
	for (i = 0; i < vol->reserved_pebs; i++) {
		ret = ubi_eba_unmap_leb(ubi, vol, i);
		if (ret)
			goto out_err;
	}

	kfree(vol->eba_tbl);
	ubi->volumes[vol->vol_id]->eba_tbl = NULL;
	ubi->volumes[vol->vol_id] = NULL;

	ubi->rsvd_pebs -= reserved_pebs;
	ubi->avail_pebs += reserved_pebs;
	i = ubi->beb_rsvd_level - ubi->beb_rsvd_pebs;
	if (i > 0) {
		i = ubi->avail_pebs >= i ? i : ubi->avail_pebs;
		ubi->avail_pebs -= i;
		ubi->rsvd_pebs += i;
		ubi->beb_rsvd_pebs += i;
	}
	ubi->vol_count -= 1;

	return 0;
out_err:
	cprintln(ERROR, "*** Failed to remove volume '%s', err = %d ***",
		 volume, ret);

	return ret;
}

static int create_ubi_volume(const char *volume, u64 size, int vol_id,
			     bool autoresize)
{
	struct ubi_mkvol_req req;
	struct ubi_device *ubi;
	int ret;

	ubi = ubi_devices[0];

	req.vol_type = UBI_DYNAMIC_VOLUME;
	req.vol_id = vol_id;
	req.alignment = 1;
	req.bytes = size;
	req.flags = 0;

	req.name_len = strlen(volume);
	if (!req.name_len || req.name_len > UBI_MAX_VOLUME_NAME)
		return -EINVAL;

	strncpy(req.name, volume, req.name_len + 1);

	if (autoresize)
		req.bytes = ubi->avail_pebs * ubi->leb_size;

	/* Call real ubi create volume */
	ret = ubi_create_volume(ubi, &req);
	if (!ret)
		return 0;

	cprintln(ERROR, "*** Failed to create volume '%s', err = %d ***",
		 volume, ret);

	return ret;
}

static int update_ubi_volume(const char *volume, int vol_id, const void *data,
			     size_t size)
{
	struct ubi_volume *vol;
	struct ubi_device *ubi;
	int ret;

	ubi = ubi_devices[0];

	printf("Updating volume '%s' from 0x%lx, size 0x%zx ... ", volume,
	       (ulong)data, size);

	vol = find_ubi_volume(volume);
	if (vol) {
		if (vol_id < 0)
			vol_id = vol->vol_id;

		ret = remove_ubi_volume(volume);
		if (ret)
			return ret;
	}

	if (vol_id < 0)
		vol_id = UBI_VOL_NUM_AUTO;

	ret = create_ubi_volume(volume, size, vol_id, false);
	if (ret)
		return ret;

	vol = find_ubi_volume(volume);
	if (!vol) {
		cprintln(ERROR, "*** Cannot find volume '%s' ***", volume);
		return -ENODEV;
	}

	ret = ubi_start_update(ubi, vol, size);
	if (ret < 0) {
		cprintln(ERROR, "*** Cannot start volume update on '%s' ***",
			 volume);
		return ret;
	}

	ret = ubi_more_update_data(ubi, vol, data, size);
	if (ret < 0) {
		cprintln(ERROR, "*** Failed to update volume '%s' ***",
			 volume);
		return ret;
	} else if (ret) {
		size = ret;

		ret = ubi_check_volume(ubi, vol->vol_id);
		if (ret < 0) {
			cprintln(ERROR, "*** Failed to update volume '%s' ***",
				 volume);
			return ret;
		}

		if (ret) {
			cprintln(ERROR, "*** Volume '%s' is corrupt ***",
				 volume);
			vol->corrupted = 1;
		}

		vol->checked = 1;
	}

	printf("OK\n");

	return 0;
}

static int read_ubi_volume(const char *volume, void *buff, size_t size)
{
	int ret, lnum, off, len, tbuf_size;
	struct ubi_device *ubi;
	unsigned long long tmp;
	struct ubi_volume *vol;
	loff_t offp = 0;
	size_t len_read;
	void *tbuf;

	ubi = ubi_devices[0];

	printf("Reading from volume '%s' to 0x%lx, size 0x%zx ... ", volume,
	       (ulong)buff, size);

	vol = find_ubi_volume(volume);
	if (!vol) {
		cprintln(ERROR, "*** Cannot find volume '%s' ***", volume);
		return -ENODEV;
	}

	if (vol->updating) {
		cprintln(ERROR, "*** Volume '%s' is updating ***", volume);
		return -EBUSY;
	}
	if (vol->upd_marker) {
		cprintln(ERROR, "*** Volume '%s' is damaged ***", volume);
		return -EBADF;
	}
	if (offp == vol->used_bytes)
		return 0;

	if (size == 0)
		size = vol->used_bytes;

	if (vol->corrupted)
		cprintln(CAUTION, "Volume '%s' is corrupted", volume);

	if (offp + size > vol->used_bytes)
		size = vol->used_bytes - offp;

	tbuf_size = vol->usable_leb_size;
	if (size < tbuf_size)
		tbuf_size = ALIGN(size, ubi->min_io_size);

	tbuf = malloc_cache_aligned(tbuf_size);
	if (!tbuf) {
		cprintln(ERROR, "*** Insufficient memory ***");
		return -ENOMEM;
	}

	len = size > tbuf_size ? tbuf_size : size;

	tmp = offp;
	off = do_div(tmp, vol->usable_leb_size);
	lnum = tmp;
	len_read = size;
	do {
		if (off + len >= vol->usable_leb_size)
			len = vol->usable_leb_size - off;

		ret = ubi_eba_read_leb(ubi, vol, lnum, tbuf, off, len, 0);
		if (ret) {
			cprintln(ERROR, "*** Error reading volume, ret = %d ***",
				 ret);
			break;
		}

		off += len;
		if (off == vol->usable_leb_size) {
			lnum += 1;
			off -= vol->usable_leb_size;
		}

		size -= len;
		offp += len;

		memcpy(buff, tbuf, len);

		buff += len;
		len = size > tbuf_size ? tbuf_size : size;
	} while (size);

	free(tbuf);

	if (!ret)
		printf("OK\n");

	return ret;
}

static int write_ubi1_tar_image(const void *data, size_t size,
				struct mtd_info *mtd_kernel,
				struct mtd_info *mtd_ubi)
{
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	int ret = 0;

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	/* Write kernel part to kernel MTD partition */
	ret = mtd_update_generic(mtd_kernel, kernel_data, kernel_size, true);
	if (ret)
		return ret;

	ret = mount_ubi(mtd_ubi);
	if (ret)
		return ret;

	/* Remove this volume first in case of no enough PEBs */
	remove_ubi_volume("rootfs_data");

	ret = update_ubi_volume("rootfs", -1, rootfs_data, rootfs_size);
	if (ret)
		goto out;

	ret = create_ubi_volume("rootfs_data", 0, -1, true);

out:
	umount_ubi();

	return ret;
}

#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
static int write_ubi2_tar_image_separate(const void *data, size_t size,
				struct mtd_info *mtd_kernel, struct mtd_info *mtd_rootfs)
{
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	int ret;

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	ret = mount_ubi(mtd_kernel);
	if (ret)
		return ret;

	ret = update_ubi_volume("kernel", -1, kernel_data, kernel_size);
	if (ret)
		goto out;

	umount_ubi();

	ret = mount_ubi(mtd_rootfs);
	if (ret)
		return ret;

	/* Remove this volume first in case of no enough PEBs */
	remove_ubi_volume("rootfs_data");

	ret = update_ubi_volume("rootfs", -1, rootfs_data, rootfs_size);
	if (ret)
		goto out;

	ret = create_ubi_volume("rootfs_data", 0, -1, true);

out:
	umount_ubi();

	return ret;
}
#endif

static int write_ubi2_tar_image(const void *data, size_t size,
				struct mtd_info *mtd)
{
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	int ret;

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	ret = mount_ubi(mtd);
	if (ret)
		return ret;

	/* Remove this volume first in case of no enough PEBs */
	remove_ubi_volume("rootfs_data");

	ret = update_ubi_volume("kernel", -1, kernel_data, kernel_size);
	if (ret)
		goto out;

	ret = update_ubi_volume("rootfs", -1, rootfs_data, rootfs_size);
	if (ret)
		goto out;

	ret = create_ubi_volume("rootfs_data", 0, -1, true);

out:
	umount_ubi();

	return ret;
}

static int write_ubi_fit_image(const void *data, size_t size,
			       struct mtd_info *mtd)
{
	int ret;

	ret = mount_ubi(mtd);
	if (ret)
		return ret;

	if (!find_ubi_volume("fit")) {
		/* ubi is dirty, erase ubi and recreate volumes */
		umount_ubi();
		ubi_mtd_param_parse(mtd->name, NULL);
		ret = mtd_erase_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, NULL, false);
		if (ret)
			return ret;

		ret = mount_ubi(mtd);
		if (ret)
			return ret;

#ifdef CONFIG_ENV_IS_IN_UBI
		ret = create_ubi_volume(CONFIG_ENV_UBI_VOLUME, CONFIG_ENV_SIZE, -1, false);
		if (ret)
			goto out;
#ifdef CONFIG_SYS_REDUNDAND_ENVIRONMENT
		ret = create_ubi_volume(CONFIG_ENV_UBI_VOLUME_REDUND, CONFIG_ENV_SIZE, -1, false);
		if (ret)
			goto out;
#endif /* CONFIG_SYS_REDUNDAND_ENVIRONMENT */
#endif /* CONFIG_ENV_IS_IN_UBI */
	}

	/* Remove this volume first in case of no enough PEBs */
	remove_ubi_volume("rootfs_data");

	ret = update_ubi_volume("fit", -1, data, size);
	if (ret)
		goto out;

	ret = create_ubi_volume("rootfs_data", 0, -1, true);

out:
	umount_ubi();

	return ret;
}

static int boot_from_ubi(struct mtd_info *mtd)
{
	ulong data_load_addr;
	int ret;

	/* Set load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
	data_load_addr = CONFIG_SYS_LOAD_ADDR;
#elif defined(CONFIG_LOADADDR)
	data_load_addr = CONFIG_LOADADDR;
#endif

	ret = mount_ubi(mtd);
	if (ret)
		return ret;

	ret = read_ubi_volume("kernel", (void *)data_load_addr, 0);
	if (ret == -ENODEV)
		ret = read_ubi_volume("fit", (void *)data_load_addr, 0);
	if (ret)
		return ret;

	return boot_from_mem(data_load_addr);
}

int ubi_upgrade_image(const void *data, size_t size)
{
	struct owrt_image_info ii;
	struct mtd_info *mtd, *mtd_kernel;
	int ret;
	const char *ubi_flash_part = "ubi";
#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
	struct mtd_info *mtd_ubikernel, *mtd_ubirootfs;
	const char *ubi_kernel_part;
	const char *ubi_rootfs_part;
#endif

	gen_mtd_probe_devices();

	mtd_kernel = get_mtd_device_nm("kernel");
	if (!IS_ERR_OR_NULL(mtd_kernel))
		put_mtd_device(mtd_kernel);
	else
		mtd_kernel = NULL;

#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
	ubi_flash_part = env_get("factory_part");
	if (!ubi_flash_part)
		ubi_flash_part = "ubi";
#endif

	mtd = get_mtd_device_nm(ubi_flash_part);
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		if (mtd_kernel && mtd_kernel->parent == mtd->parent) {
			ret = parse_image_ram(data, size, mtd->erasesize, &ii);

			if (!ret && ii.type == IMAGE_UBI1)
				return write_ubi1_image(data, size, mtd_kernel,
							mtd, &ii);

			if (!ret && ii.type == IMAGE_TAR)
				return write_ubi1_tar_image(data, size,
							    mtd_kernel, mtd);
		} else {
			ret = parse_image_ram(data, size, mtd->erasesize, &ii);

			if (ii.header_type == HEADER_FIT)
				return write_ubi_fit_image(data, size, mtd);

			if (!ret && ii.type == IMAGE_UBI2)
				return mtd_update_generic(mtd, data, size, true);

			if (!ret && ii.type == IMAGE_TAR) {
#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
				ubi_kernel_part = env_get("sysupgrade_kernel_ubipart");
				ubi_rootfs_part = env_get("sysupgrade_rootfs_ubipart");
				if (ubi_kernel_part && ubi_rootfs_part) {
					mtd_ubikernel = get_mtd_device_nm(ubi_kernel_part);
					mtd_ubirootfs = get_mtd_device_nm(ubi_rootfs_part);
					if (!IS_ERR_OR_NULL(mtd_ubikernel) && !IS_ERR_OR_NULL(mtd_ubirootfs)) {
						put_mtd_device(mtd_ubikernel);
						put_mtd_device(mtd_ubirootfs);
						return write_ubi2_tar_image_separate(data, size, mtd_ubikernel, mtd_ubirootfs);
					} else {
						return -ENOTSUPP;
					}
				}
#endif
				return write_ubi2_tar_image(data, size, mtd);
			}
		}
	}

	mtd = get_mtd_device_nm("firmware");
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		ret = parse_image_ram(data, size, mtd->erasesize, &ii);
		if (!ret && (ii.type == IMAGE_RAW || ii.type == IMAGE_UBI1))
			return mtd_update_generic(mtd, data, size, true);
	}

	return -ENOTSUPP;
}

int ubi_boot_image(void)
{
	struct mtd_info *mtd, *mtd_kernel;
	const char *ubi_boot_part = "ubi";

	gen_mtd_probe_devices();

	mtd_kernel = get_mtd_device_nm("kernel");
	if (!IS_ERR_OR_NULL(mtd_kernel))
		put_mtd_device(mtd_kernel);
	else
		mtd_kernel = NULL;

#ifdef CONFIG_MEDIATEK_MULTI_MTD_LAYOUT
	ubi_boot_part = env_get("ubi_boot_part");
	if (!ubi_boot_part)
		ubi_boot_part = "ubi";
#endif

	mtd = get_mtd_device_nm(ubi_boot_part);
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		if (mtd_kernel && mtd_kernel->parent == mtd->parent)
			return boot_from_mtd(mtd_kernel, 0);
		else
			return boot_from_ubi(mtd);
	}

	mtd = get_mtd_device_nm("firmware");
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		return boot_from_mtd(mtd, 0);
	}

	return -ENODEV;
}
