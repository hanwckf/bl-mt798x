// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt MTD-based image upgrading & booting helper
 */

#include <env.h>
#include <errno.h>
#include <image.h>
#include <memalign.h>
#include <mtd.h>
#include <ubi_uboot.h>
#include <linux/mtd/mtd.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/kernel.h>

#include "image_helper.h"
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "colored_print.h"
#include "verify_helper.h"
#include "mtd_helper.h"
#include "dual_boot.h"
#include "untar.h"

#define PART_UBI_NAME		"ubi"
#define PART_FIRMWARE_NAME	"firmware"
#define PART_KERNEL_NAME	"kernel"
#define PART_ROOTFS_NAME	"rootfs"
#define PART_ROOTFS_DATA_NAME	"rootfs_data"

#ifdef CONFIG_CMD_UBI
struct ubi_image_read_priv {
	struct image_read_priv p;
	const char *volume;
};

static const struct dual_boot_slot ubi_boot_slots[DUAL_BOOT_MAX_SLOTS] = {
	{
		.kernel = PART_KERNEL_NAME,
		.rootfs = PART_ROOTFS_NAME,
	},
#ifdef CONFIG_MTK_DUAL_BOOT
	{
		.kernel = CONFIG_MTK_DUAL_BOOT_SLOT_1_KERNEL_NAME,
		.rootfs = CONFIG_MTK_DUAL_BOOT_SLOT_1_ROOTFS_NAME,
	},
#endif /* CONFIG_MTK_DUAL_BOOT */
};

static char ubi_root_path[256];
#endif /* CONFIG_CMD_UBI */

void gen_mtd_probe_devices(void)
{
#ifdef CONFIG_MTK_FIXED_MTD_MTDPARTS
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

int mtd_erase_skip_bad(struct mtd_info *mtd, u64 offset, u64 size,
		       u64 maxsize, u64 *erasedsize, const char *name,
		       bool fullerase)
{
	u64 start, end, len, limit;
	struct erase_info ei;
	int ret;

	if (!mtd)
		return -EINVAL;

	if (!size)
		return 0;

	if (offset >= mtd->size) {
		printf("\n");
		cprintln(ERROR, "*** Erase offset pasts flash size! ***");
		return -ERANGE;
	}

	if (size > mtd->size - offset) {
		printf("\n");
		cprintln(ERROR, "*** Erase size pasts flash size! ***");
		return -ERANGE;
	}

	if (maxsize < size)
		maxsize = size;

	if (maxsize > mtd->size - offset)
		maxsize = mtd->size - offset;

	limit = (offset + maxsize + mtd->erasesize_mask) &
		(~(u64)mtd->erasesize_mask);
	end = (offset + size + mtd->erasesize_mask) &
	      (~(u64)mtd->erasesize_mask);
	start = offset & (~(u64)mtd->erasesize_mask);
	len = end - start;

	printf("Erasing '%s' from 0x%llx, size 0x%llx ... ",
	       name ? name : mtd->name, mtd->offset + start, len);

	memset(&ei, 0, sizeof(ei));

	ei.mtd = mtd;

	if (erasedsize)
		*erasedsize = 0;

	while (len && start < limit) {
		ret = mtd_block_isbad(mtd, start);
		if (ret < 0) {
			printf("Failed to check bad block at 0x%llx\n",
			       mtd->offset + start);
			return ret;
		}

		if (ret) {
			printf("Skipped bad block at 0x%llx\n",
			       mtd->offset + start);
			start += mtd->erasesize;
			continue;
		}

		ei.addr = start;
		ei.len = mtd->erasesize;

		ret = mtd_erase(mtd, &ei);
		if (ret) {
			printf("Failed at 0x%llx, err = %d\n",
			       mtd->offset + start, ret);
			return ret;
		}

		len -= mtd->erasesize;
		start += mtd->erasesize;

		if (erasedsize)
			*erasedsize += mtd->erasesize;
	}

	if (len && fullerase) {
		printf("No enough blocks erased\n");
		return -ENODATA;
	}

	printf("OK\n");

	return 0;
}

static int mtd_validate_block(struct mtd_info *mtd, u64 addr, size_t size,
			      const void *data)
{
	struct mtd_oob_ops ops;
	size_t col, chksz;
	int ret;

	static u8 *vbuff_ptr = NULL;
	static size_t vbuff_sz = 0;

	if (mtd->writesize > vbuff_sz) {
		if (!vbuff_ptr) {
			vbuff_ptr = malloc(mtd->writesize);
		} else {
			u8 *tptr = realloc(vbuff_ptr, mtd->writesize);
			if (tptr) {
				vbuff_ptr = tptr;
			} else {
				free(vbuff_ptr);
				vbuff_ptr = NULL;
			}
		}

		if (!vbuff_ptr) {
			cprintln(ERROR,
				 "*** Insufficient memory for verifying! ***");
			return -ENOMEM;
		}

		vbuff_sz = mtd->writesize;
	}

	memset(&ops, 0, sizeof(ops));

	while (size) {
		col = addr & mtd->writesize_mask;

		chksz = mtd->writesize - col;
		if (chksz > size)
			chksz = size;

		ops.mode = MTD_OPS_AUTO_OOB;
		ops.datbuf = vbuff_ptr;
		ops.len = chksz;
		ops.retlen = 0;

		ret = mtd_read_oob(mtd, addr, &ops);
		if (ret && ret != -EUCLEAN) {
			printf("Failed to read at 0x%llx, err = %d\n",
			       mtd->offset + addr, ret);
			return ret;
		}

		if (!verify_data(data, vbuff_ptr, mtd->offset + addr, chksz))
			return -EBADMSG;

		size -= chksz;
		addr += chksz;
		data = (char *)data + chksz;
	}

	return 0;
}

int mtd_write_skip_bad(struct mtd_info *mtd, u64 offset, size_t size,
		       u64 maxsize, size_t *writtensize, const void *data,
		       bool verify)
{
	struct mtd_oob_ops ops;
	bool checkbad = true;
	size_t len, chksz;
	u64 addr, limit;
	u32 blockoff;
	int ret;

	if (!mtd)
		return -EINVAL;

	if (!size)
		return 0;

	if (offset >= mtd->size) {
		printf("\n");
		cprintln(ERROR, "*** Write offset pasts flash size! ***");
		return -ERANGE;
	}

	if (maxsize < size)
		maxsize = size;

	if (maxsize > mtd->size - offset)
		maxsize = mtd->size - offset;

	if (check_data_size(mtd->size, offset, maxsize, size, true))
		return -EINVAL;

	printf("Writing '%s' from 0x%lx to 0x%llx, size 0x%zx ... ", mtd->name,
	       (ulong)data, mtd->offset + offset, size);

	limit = offset + maxsize;
	addr = offset;
	len = size;

	memset(&ops, 0, sizeof(ops));

	if (writtensize)
		*writtensize = 0;

	while (len && addr < limit) {
		if (checkbad || !(addr & mtd->erasesize_mask)) {
			ret = mtd_block_isbad(mtd, addr);
			if (ret < 0) {
				printf("Failed to check bad block at 0x%llx\n",
				       mtd->offset + addr);
				return ret;
			}

			if (ret) {
				printf("Skipped bad block at 0x%llx\n",
				       mtd->offset + addr);
				addr += mtd->erasesize;
				checkbad = true;
				continue;
			}

			checkbad = false;
		}

		blockoff = addr & mtd->erasesize_mask;

		chksz = mtd->erasesize - blockoff;
		if (chksz > len)
			chksz = len;

		if (addr + chksz > limit)
			chksz = limit - addr;

		ops.mode = MTD_OPS_AUTO_OOB;
		ops.datbuf = (void *)data;
		ops.len = chksz;
		ops.retlen = 0;

		ret = mtd_write_oob(mtd, addr, &ops);
		if (ret) {
			printf("Failed at 0x%llx, err = %d\n",
			       mtd->offset + addr, ret);
			return ret;
		}

		if (verify) {
			ret = mtd_validate_block(mtd, addr, chksz, data);
			if (ret)
				return ret;
		}

		addr += ops.retlen;
		len -= ops.retlen;
		data += ops.retlen;

		if (writtensize)
			*writtensize += ops.retlen;
	}

	if (len) {
		printf("Incomplete write\n");
		return -ENODATA;
	}

	printf("OK\n");

	return 0;
}

int mtd_read_skip_bad(struct mtd_info *mtd, u64 offset, size_t size,
		      u64 maxsize, size_t *readsize, void *data)
{
	struct mtd_oob_ops ops;
	bool checkbad = true;
	size_t len, chksz;
	u64 addr, limit;
	u32 blockoff;
	int ret;

	if (!mtd)
		return -EINVAL;

	if (!size)
		return 0;

	if (offset >= mtd->size) {
		printf("\n");
		cprintln(ERROR, "*** Read offset pasts flash size! ***");
		return -ERANGE;
	}

	if (maxsize < size)
		maxsize = size;

	if (maxsize > mtd->size - offset)
		maxsize = mtd->size - offset;

	if (check_data_size(mtd->size, offset, maxsize, size, false))
		return -EINVAL;

	printf("Reading '%s' from 0x%llx to 0x%lx, size 0x%zx ... ", mtd->name,
	       mtd->offset + offset, (ulong)data, size);

	limit = offset + maxsize;
	addr = offset;
	len = size;

	memset(&ops, 0, sizeof(ops));

	if (readsize)
		*readsize = 0;

	while (len && addr < limit) {
		if (checkbad || !(addr & mtd->erasesize_mask)) {
			ret = mtd_block_isbad(mtd, addr);
			if (ret < 0) {
				printf("Failed to check bad block at 0x%llx\n",
				       mtd->offset + addr);
				return ret;
			}

			if (ret) {
				printf("Skipped bad block at 0x%llx\n",
				       mtd->offset + addr);
				addr += mtd->erasesize;
				checkbad = true;
				continue;
			}

			checkbad = false;
		}

		blockoff = addr & mtd->erasesize_mask;

		chksz = mtd->erasesize - blockoff;
		if (chksz > len)
			chksz = len;

		if (addr + chksz > limit)
			chksz = limit - addr;

		ops.mode = MTD_OPS_AUTO_OOB;
		ops.datbuf = data;
		ops.len = chksz;
		ops.retlen = 0;

		ret = mtd_read_oob(mtd, addr, &ops);
		if (ret && ret != -EUCLEAN) {
			printf("Failed at 0x%llx, err = %d\n",
			       mtd->offset + addr, ret);
			return ret;
		}

		addr += ops.retlen;
		len -= ops.retlen;
		data += ops.retlen;

		if (readsize)
			*readsize += ops.retlen;
	}

	if (len) {
		printf("Incomplete read\n");
		return -ENODATA;
	}

	printf("OK\n");

	return 0;
}

int mtd_update_generic(struct mtd_info *mtd, const void *data, size_t size,
		       bool verify)
{
	int ret;

	ret = mtd_erase_skip_bad(mtd, 0, size, mtd->size, NULL, NULL, true);
	if (ret)
		return ret;

	return mtd_write_skip_bad(mtd, 0, size, mtd->size, NULL, data, verify);
}

int boot_from_mtd(struct mtd_info *mtd, u64 offset)
{
	u32 fit_hdrsize = sizeof(struct fdt_header);
	u32 legacy_hdrsize = image_get_header_size();
	ulong data_load_addr;
	u32 size;
	int ret;

	/* Set load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
	data_load_addr = CONFIG_SYS_LOAD_ADDR;
#elif defined(CONFIG_LOADADDR)
	data_load_addr = CONFIG_LOADADDR;
#endif

	size = max3(fit_hdrsize, legacy_hdrsize, mtd->writesize);

	ret = mtd_read_skip_bad(mtd, offset, size, mtd->size, NULL,
				(void *)data_load_addr);
	if (ret)
		return ret;

	switch (genimg_get_format((const void *)data_load_addr)) {
#if defined(CONFIG_LEGACY_IMAGE_FORMAT)
	case IMAGE_FORMAT_LEGACY:
		size = image_get_image_size((const void *)data_load_addr);
		break;
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		size = fit_get_size((const void *)data_load_addr);
		break;
#endif
	default:
		printf("Error: no Image found in %s at 0x%llx\n", mtd->name,
		       offset);
		return -EINVAL;
	}

	ret = mtd_read_skip_bad(mtd, offset, size, mtd->size, NULL,
				(void *)data_load_addr);
	if (ret)
		return ret;

	return boot_from_mem(data_load_addr);
}

#ifdef CONFIG_CMD_UBI
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

static int mount_ubi(struct mtd_info *mtd, bool create)
{
	int ret;

	ret = ubi_part(mtd->name, NULL);
	if (ret) {
		if (create) {
			cprintln(CAUTION, "*** Failed to attach UBI ***");
			cprintln(NORMAL, "*** Rebuilding UBI ***");

			ret = mtd_erase_skip_bad(mtd, 0, mtd->size, mtd->size,
						 NULL, NULL, false);
			if (ret)
				return ret;

			ret = ubi_init();
		}

		if (ret) {
			cprintln(ERROR, "*** Failed to attach UBI ***");
			return -ret;
		}
	}

	return 0;
}

static int remove_ubi_volume(const char *volume)
{
	return ubi_remove_vol((char *)volume);
}

static int create_ubi_volume(const char *volume, u64 size, int vol_id,
			     bool autoresize)
{
	int ret;

	ret = ubi_create_vol((char *)volume, autoresize ? -1 : size, 1, vol_id,
			     false);
	if (!ret)
		return 0;

	cprintln(ERROR, "*** Failed to create volume '%s', err = %d ***",
		 volume, ret);

	return ret;
}

static int update_ubi_volume_custom(const char *volume, int vol_id,
				    const void *data, size_t size,
				    uint64_t reserved_size, bool dynamic)
{
	struct ubi_volume *vol;
	int ret;

	vol = ubi_find_volume((char *)volume);
	if (vol) {
		if (vol_id < 0)
			vol_id = vol->vol_id;

		ret = remove_ubi_volume(volume);
		if (ret)
			return ret;
	}

	if (vol_id < 0)
		vol_id = UBI_VOL_NUM_AUTO;

	if (reserved_size < size)
		reserved_size = size;

	ret = ubi_create_vol((char *)volume, reserved_size, dynamic, vol_id,
			     false);
	if (ret)
		return ret;

	printf("Updating volume '%s' from 0x%lx, size 0x%zx ... ", volume,
	       (ulong)data, size);

	ret = ubi_volume_write((char *)volume, (void *)data, size);
	if (!ret) {
		printf("OK\n");
		return 0;
	}

	return ret;
}

static int update_ubi_volume(const char *volume, int vol_id, const void *data,
			     size_t size)
{
	return update_ubi_volume_custom(volume, vol_id, data, size, size, true);
}

static int read_ubi_volume(const char *volume, void *buff, size_t size)
{
	return ubi_volume_read((char *)volume, buff, size);
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

	ret = mount_ubi(mtd_ubi, true);
	if (ret)
		return ret;

	/* Remove this volume first in case of no enough PEBs */
	remove_ubi_volume(PART_ROOTFS_DATA_NAME);

	ret = update_ubi_volume(PART_ROOTFS_NAME, -1, rootfs_data, rootfs_size);
	if (ret)
		return ret;

	return create_ubi_volume(PART_ROOTFS_DATA_NAME, 0, -1, true);
}

static int write_ubi2_tar_image(const void *data, size_t size,
				struct mtd_info *mtd)
{
	const char *kernel_part, *rootfs_part;
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	u32 slot;
	int ret;

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
	struct ubi_volume *vol;
#endif

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT)) {
		slot = dual_boot_get_next_slot();
		printf("Upgrading image slot %u ...\n", slot);

		kernel_part = ubi_boot_slots[slot].kernel;
		rootfs_part = ubi_boot_slots[slot].rootfs;
	} else {
		kernel_part = PART_KERNEL_NAME;
		rootfs_part = PART_ROOTFS_NAME;
	}

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	ret = mount_ubi(mtd, !IS_ENABLED(CONFIG_MTK_DUAL_BOOT));
	if (ret)
		return ret;

#ifndef CONFIG_MTK_DUAL_BOOT
	/* Remove this volume first in case of no enough PEBs */
	remove_ubi_volume(PART_ROOTFS_DATA_NAME);
#endif

	ret = update_ubi_volume(kernel_part, -1, kernel_data, kernel_size);
	if (ret)
		return ret;

	ret = update_ubi_volume(rootfs_part, -1, rootfs_data, rootfs_size);
	if (ret)
		return ret;

#ifdef CONFIG_MTK_DUAL_BOOT
	ret = dual_boot_set_slot_invalid(slot, false, false);
	if (ret)
		printf("Error: failed to set new image slot valid in env\n");

	ret = dual_boot_set_current_slot(slot);
	if (ret)
		printf("Error: failed to save new image slot to env\n");

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
	if (!IS_ENABLED(CONFIG_MTK_DUAL_BOOT_RESERVE_ROOTFS_DATA)) {
		/* If we do not reserve rootfs_data, just recreate it */
		remove_ubi_volume(CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME);
	}

	vol = ubi_find_volume((char *)CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME);
	if (!vol) {
		ret = create_ubi_volume(CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME,
					CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_SIZE << 20,
					-1, false);
	}
#endif

	return ret;
#else
	return create_ubi_volume(PART_ROOTFS_DATA_NAME, 0, -1, true);
#endif /* CONFIG_MTK_DUAL_BOOT */
}

static int ubi_image_read(struct image_read_priv *rpriv, void *buff, u64 addr,
			  size_t size)
{
	struct ubi_image_read_priv *priv =
		container_of(rpriv, struct ubi_image_read_priv, p);

	if (addr) {
		printf("Reading from non-zero offset within a volume is not allowed.\n");
		return -ENOTSUPP;
	}

	return read_ubi_volume(priv->volume, buff, size);
}

static int ubi_boot_verify(const struct dual_boot_slot *slot, ulong loadaddr)
{
	struct fit_hashes kernel_hashes, rootfs_hashes;
	struct ubi_image_read_priv read_priv;
	size_t kernel_size, rootfs_size;
	void *kernel_data, *rootfs_data;
	bool ret;

	read_priv.p.page_size = 1;
	read_priv.p.block_size = 0;
	read_priv.p.read = ubi_image_read;

	/* Verify kernel first */
	read_priv.volume = slot->kernel;
	kernel_data = (void *)loadaddr;
	ret = read_verify_kernel(&read_priv.p, kernel_data, 0, 0,
				 &kernel_size, &kernel_hashes);
	if (!ret) {
		printf("Error: kernel verification failed\n");
		return -EBADMSG;
	}

	/* Verify rootfs, requiring valid kernel FIT image */
	read_priv.volume = slot->rootfs;
	rootfs_data = kernel_data + ALIGN(kernel_size, 4);
	ret = read_verify_rootfs(&read_priv.p, kernel_data, rootfs_data,
				 0, 0, &rootfs_size, false, NULL,
				 &rootfs_hashes);
	if (!ret) {
		printf("Error: rootfs verification failed\n");
		return -EBADMSG;
	}

	return 0;
}

static int ubi_set_bootargs(void)
{
	struct ubi_volume *vol;
	u32 slot;
	int ret;

	bootargs_reset();

	slot = dual_boot_get_current_slot();

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_RESERVE_ROOTFS_DATA)) {
		ret = bootargs_set("boot_param.reserve_rootfs_data", NULL);
		if (ret)
			return ret;
	}

	ret = bootargs_set("ubi.rootfs_volume", ubi_boot_slots[slot].rootfs);
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_MTK_SECURE_BOOT)) {
		vol = ubi_find_volume((char *)ubi_boot_slots[slot].rootfs);
		if (!vol)
			return -ENODEV;

		snprintf(ubi_root_path, sizeof(ubi_root_path),
			 "/dev/ubiblock%u_%u", ubi_devices[0]->ubi_num,
			 vol->vol_id);

		ret = bootargs_set("root", ubi_root_path);
		if (ret)
			return ret;

		ret = bootargs_set("ubi.no_default_rootdev", NULL);
		if (ret)
			return ret;
	}

	ret = bootargs_set("boot_param.boot_kernel_part",
			   ubi_boot_slots[slot].kernel);
	if (ret)
		return ret;

	ret = bootargs_set("boot_param.boot_rootfs_part",
			   ubi_boot_slots[slot].rootfs);
	if (ret)
		return ret;

	slot = dual_boot_get_next_slot();

	ret = bootargs_set("boot_param.upgrade_kernel_part",
			   ubi_boot_slots[slot].kernel);
	if (ret)
		return ret;

	ret = bootargs_set("boot_param.upgrade_rootfs_part",
			   ubi_boot_slots[slot].rootfs);
	if (ret)
		return ret;

#ifdef CONFIG_ENV_IS_IN_UBI
	ret = bootargs_set("boot_param.env_part", CONFIG_ENV_UBI_VOLUME);
	if (ret)
		return ret;
#endif

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
	ret = bootargs_set("boot_param.rootfs_data_part",
			   CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME);
	if (ret)
		return ret;
#endif

	return 0;
}

static int ubi_boot_slot(uint32_t slot)
{
	ulong data_load_addr = CONFIG_SYS_LOAD_ADDR;
	int ret;

	printf("Trying to boot from image slot %u\n", slot);

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_IMAGE_ROOTFS_VERIFY)) {
		ret = ubi_boot_verify(&ubi_boot_slots[slot], data_load_addr);
		if (ret) {
			printf("Firmware integrity verification failed\n");
			return ret;
		}

		printf("Firmware integrity verification passed\n");
	} else {
		ret = read_ubi_volume(ubi_boot_slots[slot].kernel,
				      (void *)data_load_addr, 0);
		if (ret)
			return ret;
	}

	return boot_from_mem(data_load_addr);
}

static int ubi_dual_boot(struct mtd_info *mtd)
{
	u32 slot;
	int ret;

	ret = mount_ubi(mtd, false);
	if (ret)
		return ret;

	slot = dual_boot_get_current_slot();

	if (dual_boot_is_slot_invalid(slot)) {
		printf("Image slot %u was marked invalid.\n", slot);
	} else {
		ubi_set_bootargs();

		ret = ubi_boot_slot(slot);

		printf("Failed to boot from current image slot, error %d\n", ret);

		dual_boot_set_slot_invalid(slot, true, true);
	}

	slot = dual_boot_get_next_slot();

	if (dual_boot_is_slot_invalid(slot)) {
		printf("Image slot %u was marked invalid.\n", slot);
	} else {
		ret = dual_boot_set_current_slot(slot);
		if (ret) {
			panic("Error: failed to set new image boot slot, error %d\n",
			      ret);
			return ret;
		}

		ubi_set_bootargs();

		ret = ubi_boot_slot(slot);

		printf("Failed to boot from next image slot, error %d\n", ret);

		dual_boot_set_slot_invalid(slot, true, true);
	}

	return ret;
}

static int boot_from_ubi(struct mtd_info *mtd)
{
	ulong data_load_addr = CONFIG_SYS_LOAD_ADDR;
	int ret;

	ret = mount_ubi(mtd, false);
	if (ret)
		return ret;

	ret = read_ubi_volume(PART_KERNEL_NAME, (void *)data_load_addr, 0);
	if (ret)
		return ret;

	return boot_from_mem(data_load_addr);
}

int ubi_upgrade_volume(const char *name, const void *data, size_t size,
		       uint64_t reserved_size, bool dynamic)
{
	struct mtd_info *mtd;
	int ret;

	gen_mtd_probe_devices();

	mtd = get_mtd_device_nm(PART_UBI_NAME);
	if (IS_ERR_OR_NULL(mtd)) {
		cprintln(ERROR, "*** Partition %s does not exist ***", PART_UBI_NAME);
		return -ENODEV;
	}

	put_mtd_device(mtd);

	ret = mount_ubi(mtd, !IS_ENABLED(CONFIG_MTK_DUAL_BOOT));
	if (ret)
		return ret;

	return update_ubi_volume_custom(name, -1, data, size, reserved_size,
					dynamic);
}
#endif /* CONFIG_CMD_UBI */

int mtd_upgrade_image(const void *data, size_t size)
{
	struct owrt_image_info ii;
	struct mtd_info *mtd;
	int ret;

#ifdef CONFIG_CMD_UBI
	struct mtd_info *mtd_kernel;
#endif /* CONFIG_CMD_UBI */

	gen_mtd_probe_devices();

#ifdef CONFIG_CMD_UBI
	mtd_kernel = get_mtd_device_nm(PART_KERNEL_NAME);
	if (!IS_ERR_OR_NULL(mtd_kernel))
		put_mtd_device(mtd_kernel);
	else
		mtd_kernel = NULL;

	mtd = get_mtd_device_nm(PART_UBI_NAME);
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		if (mtd_kernel && mtd_kernel->parent == mtd->parent &&
		    !IS_ENABLED(CONFIG_MTK_DUAL_BOOT)) {
			ret = parse_image_ram(data, size, mtd->erasesize, &ii);

			if (!ret && ii.type == IMAGE_UBI1)
				return write_ubi1_image(data, size, mtd_kernel,
							mtd, &ii);

			if (!ret && ii.type == IMAGE_TAR)
				return write_ubi1_tar_image(data, size,
							    mtd_kernel, mtd);
		} else {
			ret = parse_image_ram(data, size, mtd->erasesize, &ii);

			if (!ret && ii.type == IMAGE_UBI2 &&
			    !IS_ENABLED(CONFIG_MTK_DUAL_BOOT))
				return mtd_update_generic(mtd, data, size, true);

			if (!ret && ii.type == IMAGE_TAR)
				return write_ubi2_tar_image(data, size, mtd);
		}
	}
#endif /* CONFIG_CMD_UBI */

#ifndef CONFIG_MTK_DUAL_BOOT
	mtd = get_mtd_device_nm(PART_FIRMWARE_NAME);
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		ret = parse_image_ram(data, size, mtd->erasesize, &ii);
		if (!ret && (ii.type == IMAGE_RAW || ii.type == IMAGE_UBI1)) {
			return mtd_update_generic(mtd, data, size,
				mtd->type == MTD_NANDFLASH ||
				mtd->type == MTD_MLCNANDFLASH);
		}
	}
#endif

	return -ENOTSUPP;
}

int mtd_boot_image(void)
{
	struct mtd_info *mtd;

#ifdef CONFIG_CMD_UBI
	struct mtd_info *mtd_kernel;
#endif /* CONFIG_CMD_UBI */

	gen_mtd_probe_devices();

#ifdef CONFIG_CMD_UBI
	mtd_kernel = get_mtd_device_nm(PART_KERNEL_NAME);
	if (!IS_ERR_OR_NULL(mtd_kernel))
		put_mtd_device(mtd_kernel);
	else
		mtd_kernel = NULL;

	mtd = get_mtd_device_nm(PART_UBI_NAME);
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		if (mtd_kernel && mtd_kernel->parent == mtd->parent) {
			if (!IS_ENABLED(CONFIG_MTK_DUAL_BOOT))
				return boot_from_mtd(mtd_kernel, 0);
		} else {
			if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT))
				return ubi_dual_boot(mtd);

			return boot_from_ubi(mtd);
		}
	}
#endif /* CONFIG_CMD_UBI */

#ifndef CONFIG_MTK_DUAL_BOOT
	mtd = get_mtd_device_nm(PART_FIRMWARE_NAME);
	if (!IS_ERR_OR_NULL(mtd)) {
		put_mtd_device(mtd);

		return boot_from_mtd(mtd, 0);
	}
#endif

	return -ENODEV;
}
