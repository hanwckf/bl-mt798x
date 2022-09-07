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

void ubi_probe_mtd_devices(void)
{
#ifdef CONFIG_MEDIATEK_UBI_FIXED_MTDPARTS
	const char *mtdids = NULL, *mtdparts = NULL;

#if defined(CONFIG_SYS_MTDPARTS_RUNTIME)
	board_mtdparts_default(&mtdids, &mtdparts);
#else
#if defined(MTDIDS_DEFAULT)
	mtdids = MTDIDS_DEFAULT;
#elif defined(CONFIG_MTDIDS_DEFAULT)
	mtdids = CONFIG_MTDIDS_DEFAULT;
#endif

#if defined(MTDPARTS_DEFAULT)
	mtdparts = MTDPARTS_DEFAULT;
#elif defined(CONFIG_MTDPARTS_DEFAULT)
	mtdparts = CONFIG_MTDPARTS_DEFAULT;
#endif
#endif

	if (mtdids)
		env_set("mtdids", mtdids);

	if (mtdparts)
		env_set("mtdparts", mtdparts);
#endif

	mtd_probe_devices();
}

static int mtd_update_generic(struct mtd_info *mtd, const void *data,
			      size_t size)
{
	int ret;

	/* Write ubi part to kernel MTD partition */
	ret = mtd_erase_generic(mtd, 0, size);
	if (ret)
		return ret;

	return mtd_write_generic(mtd, 0, 0, data, size, true);
}

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
	ret = mtd_update_generic(mtd_kernel, data, mtd_kernel->size);
	if (ret)
		return ret;

	/* Write ubi part to kernel MTD partition */
	return mtd_update_generic(mtd_ubi, data + mtd_kernel->size,
				  ii->ubi_size + ii->marker_size);
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

		ret = mtd_erase_generic(mtd, 0, mtd->size);
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
	ret = mtd_update_generic(mtd_kernel, kernel_data, kernel_size);
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
	if (ret)
		return ret;

	return boot_from_mem(data_load_addr);
}

int ubi_upgrade_image(const void *data, size_t size)
{
	struct owrt_image_info ii;
	struct mtd_info *mtd, *mtd_kernel;
	int ret;

	ubi_probe_mtd_devices();

	mtd_kernel = get_mtd_device_nm("kernel");
	if (!IS_ERR_OR_NULL(mtd_kernel))
		put_mtd_device(mtd_kernel);
	else
		mtd_kernel = NULL;

	mtd = get_mtd_device_nm("ubi");
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

			if (!ret && ii.type == IMAGE_UBI2)
				return mtd_update_generic(mtd, data, size);

			if (!ret && ii.type == IMAGE_TAR)
				return write_ubi2_tar_image(data, size, mtd);
		}
	}

	mtd = get_mtd_device_nm("firmware");
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		ret = parse_image_ram(data, size, mtd->erasesize, &ii);
		if (!ret && (ii.type == IMAGE_RAW || ii.type == IMAGE_UBI1))
			return mtd_update_generic(mtd, data, size);
	}

	return -ENOTSUPP;
}

int ubi_boot_image(void)
{
	struct mtd_info *mtd, *mtd_kernel;

	ubi_probe_mtd_devices();

	mtd_kernel = get_mtd_device_nm("kernel");
	if (!IS_ERR_OR_NULL(mtd_kernel))
		put_mtd_device(mtd_kernel);
	else
		mtd_kernel = NULL;

	mtd = get_mtd_device_nm("ubi");
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
