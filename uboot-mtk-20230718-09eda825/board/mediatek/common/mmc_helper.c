// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt SD/eMMC image upgrading & booting helper
 */

#include <errno.h>
#include <linux/mtd/mtd.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/kernel.h>
#include <u-boot/crc.h>
#include <memalign.h>
#include <image.h>
#include <part_efi.h>
#include <part.h>
#include <env.h>
#include <blk.h>
#include <mmc.h>

#include "image_helper.h"
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "colored_print.h"
#include "verify_helper.h"
#include "mmc_helper.h"
#include "dual_boot.h"
#include "untar.h"

/*
 * Keep this value synchronized with definition in libfstools/rootdisk.c of
 * fstools package
 */
#define ROOTDEV_OVERLAY_ALIGN	(64ULL * 1024ULL)

#define PART_KERNEL_NAME	"kernel"
#define PART_ROOTFS_NAME	"rootfs"

struct mmc_image_read_priv {
	struct image_read_priv p;
	struct mmc *mmc;
	const char *part;
};

static const struct dual_boot_slot mmc_boot_slots[DUAL_BOOT_MAX_SLOTS] = {
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

#define BOOT_PARAM_STR_MAX_LEN			256

static char boot_kernel_part[BOOT_PARAM_STR_MAX_LEN];
static char boot_rootfs_part[BOOT_PARAM_STR_MAX_LEN];
static char upgrade_kernel_part[BOOT_PARAM_STR_MAX_LEN];
static char upgrade_rootfs_part[BOOT_PARAM_STR_MAX_LEN];
static char env_part[BOOT_PARAM_STR_MAX_LEN];

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
static char rootfs_data_part[BOOT_PARAM_STR_MAX_LEN];
#endif

struct mmc *_mmc_get_dev(u32 dev, int hwpart, bool write)
{
	struct mmc *mmc;
	int ret;

#ifdef CONFIG_BLOCK_CACHE
	struct blk_desc *bd;
#endif

	mmc = find_mmc_device(dev);
	if (!mmc) {
		cprintln(ERROR, "*** MMC device %u not found! ***", dev);
		return NULL;
	}

	mmc->has_init = 0;

	ret = mmc_init(mmc);
	if (ret) {
		cprintln(ERROR, "*** Failed to initialize MMC device %u! ***",
			 dev);
		return NULL;
	}

#ifdef CONFIG_BLOCK_CACHE
	bd = mmc_get_blk_desc(mmc);
	blkcache_invalidate(bd->uclass_id, bd->devnum);
#endif

	if (write && mmc_getwp(mmc)) {
		cprintln(ERROR, "*** MMC device %u is write-protected! ***",
			 dev);
		return NULL;
	}

	if (hwpart >= 0 && mmc->part_config != MMCPART_NOAVAILABLE) {
		ret = blk_select_hwpart_devnum(UCLASS_MMC, dev, hwpart);
		if (ret || mmc_get_blk_desc(mmc)->hwpart != hwpart) {
			cprintln(ERROR,
				 "*** Failed to switch to MMC device %u part %u! ***",
				 dev, hwpart);
			return NULL;
		}
	}

	return mmc;
}

int _mmc_write(struct mmc *mmc, u64 offset, size_t max_size, const void *data,
	       size_t size, bool verify)
{
	u8 vbuff[MMC_MAX_BLOCK_LEN * 4];
	size_t size_left, chksz;
	u64 verify_offset;
	u32 blks, n;

	if (check_data_size(mmc->capacity, offset, max_size, size, true))
		return -EINVAL;

	if (offset % mmc->write_bl_len) {
		cprintln(ERROR, "*** Write offset is not aligned! ***");
		return -EINVAL;
	}

	blks = (size + mmc->write_bl_len - 1) / mmc->write_bl_len;

	printf("Writing from 0x%lx to 0x%llx, size 0x%zx ... ", (ulong)data,
	       offset, size);

	n = blk_dwrite(mmc_get_blk_desc(mmc), offset / mmc->write_bl_len, blks,
		       data);
	if (n != blks) {
		printf("Fail\n");
		cprintln(ERROR, "*** Only 0x%zx written! ***",
			 (size_t)n * mmc->write_bl_len);
		return -EIO;
	}

	printf("OK\n");

	if (!verify)
		return 0;

	printf("Verifying from 0x%llx to 0x%llx, size 0x%zx ... ", offset,
	       offset + size - 1, size);

	size_left = size;
	verify_offset = 0;

	while (size_left) {
		chksz = min(sizeof(vbuff), size_left);
		blks = (chksz + mmc->read_bl_len - 1) / mmc->read_bl_len;

		n = blk_dread(mmc_get_blk_desc(mmc),
			      (offset + verify_offset) / mmc->read_bl_len,
			      blks, vbuff);

		if (n != blks) {
			printf("Fail\n");
			cprintln(ERROR, "*** Only 0x%zx read! ***",
				 (size_t)n * mmc->read_bl_len);
			return -EIO;
		}

		if (!verify_data(data + verify_offset, vbuff,
				 offset + verify_offset, chksz))
			return -EIO;

		verify_offset += chksz;
		size_left -= chksz;
	}

	printf("OK\n");

	return 0;
}

int _mmc_read(struct mmc *mmc, u64 offset, void *data, size_t size)
{
	u8 rbuff[MMC_MAX_BLOCK_LEN];
	u32 blks, n;
	size_t chksz;

	if (check_data_size(mmc->capacity, offset, 0, size, false))
		return -EINVAL;

	printf("Reading from 0x%llx to 0x%lx, size 0x%zx ... ", offset,
	       (ulong)data, size);

	/* Unaligned access */
	if (offset % mmc->read_bl_len) {
		chksz = mmc->read_bl_len - (offset % mmc->read_bl_len);
		chksz = min(chksz, size);

		n = blk_dread(mmc_get_blk_desc(mmc), offset / mmc->read_bl_len,
			       1, rbuff);
		if (n != 1) {
			printf("Fail\n");
			cprintln(ERROR, "*** Failed to read a block! ***");
			return -EIO;
		}

		memcpy(data, rbuff, chksz);
		offset += chksz;
		data += chksz;
		size -= chksz;

		if (!size)
			goto success;
	}

	if (size >= mmc->read_bl_len) {
		blks = size / mmc->read_bl_len;
		chksz = (size_t)blks * mmc->read_bl_len;

		n = blk_dread(mmc_get_blk_desc(mmc), offset / mmc->read_bl_len,
			       blks, data);

		if (n != blks) {
			printf("Fail\n");
			cprintln(ERROR, "*** Only 0x%zx read! ***",
				 (size_t)n * mmc->read_bl_len);
			return -EIO;
		}

		offset += chksz;
		data += chksz;
		size -= chksz;
	}

	/* Data left less than 1 block */
	if (size) {
		n = blk_dread(mmc_get_blk_desc(mmc), offset / mmc->read_bl_len,
			       1, rbuff);
		if (n != 1) {
			printf("Fail\n");
			cprintln(ERROR, "*** Failed to read a block! ***");
			return -EIO;
		}

		memcpy(data, rbuff, size);
	}

success:
	printf("OK\n");

	return 0;
}

static ulong _mmc_erase_real(struct mmc *mmc, u32 start_blk, u32 blks)
{
	return blk_derase(mmc_get_blk_desc(mmc), start_blk, blks);
}

static ulong _mmc_erase_pseudo(struct mmc *mmc, u32 start_blk, u32 blks)
{
	u8 wbuff[MMC_MAX_BLOCK_LEN];
	u32 i, n, c = 0;

	memset(wbuff, 0, sizeof(wbuff));

	for (i = start_blk; i < start_blk + blks; i++) {
		n = blk_dwrite(mmc_get_blk_desc(mmc), i, 1, wbuff);
		if (n != 1)
			break;
		c++;
	}

	return c;
}

int _mmc_erase(struct mmc *mmc, u64 offset, size_t max_size, u64 size)
{
	u32 start_blk, blks, rem, n;

	if (check_data_size(mmc->capacity, offset, max_size, size, true))
		return -EINVAL;

	start_blk = offset / mmc->write_bl_len;
	blks = size / mmc->write_bl_len;

	/* unaligned blocks */
	rem = start_blk % mmc->erase_grp_size;
	if (rem)
		rem = mmc->erase_grp_size - rem;
	if (rem > blks)
		rem = blks;

	if (rem) {
		n = _mmc_erase_pseudo(mmc, start_blk, rem);
		if (n != rem)
			return -EIO;

		start_blk += rem;
		blks -= rem;
	}

	/* erase group */
	rem = (blks / mmc->erase_grp_size) * mmc->erase_grp_size;

	if (rem) {
		n = _mmc_erase_real(mmc, start_blk, rem);
		if (n != rem)
			return -EIO;

		start_blk += rem;
		blks -= rem;
	}

	/* remained blocks */
	if (blks) {
		n = _mmc_erase_pseudo(mmc, start_blk, blks);
		if (n != blks)
			return -EIO;
	}

	return 0;
}

static int _mmc_erase_env(struct mmc *mmc, u64 offset, u64 size)
{
	int ret;

	printf("Erasing environment from 0x%llx to 0x%llx, size 0x%llx ... ",
	       offset, offset + size - 1, size);

	ret = _mmc_erase(mmc, offset, 0, size);
	if (ret) {
		printf("Fail\n");
		cprintln(ERROR, "*** Failed to erase a block! ***");
		return ret;
	}

	printf("OK\n");

	return 0;
}

int mmc_write_generic(u32 dev, int hwpart, u64 offset, size_t max_size,
		      const void *data, size_t size, bool verify)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	return _mmc_write(mmc, offset, max_size, data, size, verify);
}

int mmc_read_generic(u32 dev, int hwpart, u64 offset, void *data, size_t size)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, hwpart, false);
	if (!mmc)
		return -ENODEV;

	return _mmc_read(mmc, offset, data, size);
}

int mmc_erase_generic(u32 dev, int hwpart, u64 offset, u64 size)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	return _mmc_erase(mmc, offset, 0, size);
}

int mmc_erase_env_generic(u32 dev, int hwpart, u64 offset, u64 size)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	return _mmc_erase_env(mmc, offset, size);
}

#ifdef CONFIG_PARTITIONS
int _mmc_find_part(struct mmc *mmc, const char *part_name,
		   struct disk_partition *dpart)
{
	struct blk_desc *mmc_bdev;
	bool found = false;
	u32 i = 1, namelen;
	int ret;

	mmc_bdev = mmc_get_blk_desc(mmc);
	if (!mmc_bdev || mmc_bdev->type == DEV_TYPE_UNKNOWN) {
		cprintln(ERROR, "*** Failed to get MMC block device! ***");
		return -EIO;
	}

	part_init(mmc_bdev);

	while (true) {
		ret = part_get_info(mmc_bdev, i, dpart);
		if (ret)
			break;

		namelen = strnlen((const char *)dpart->name,
				  sizeof(dpart->name));

		if (strlen(part_name) == namelen) {
			if (!strncmp((const char *)dpart->name, part_name,
				     namelen)) {
				found = true;
				break;
			}
		}

		i++;
	}

	if (!found) {
		cprintln(ERROR, "*** Partition '%s' not found! ***", part_name);
		cprintln(ERROR, "*** Please update GPT first! ***");
		return -ENODEV;
	}

	return 0;
}

int mmc_write_part(u32 dev, int hwpart, const char *part_name, const void *data,
		   size_t size, bool verify)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	return _mmc_write(mmc, (u64)dpart.start * dpart.blksz,
			  (u64)dpart.size * dpart.blksz, data, size, verify);
}

int mmc_read_part(u32 dev, int hwpart, const char *part_name, void *data,
		  size_t size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, false);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	return _mmc_read(mmc, (u64)dpart.start * dpart.blksz, data, size);
}

int mmc_read_part_size(u32 dev, int hwpart, const char *part_name, u64 *size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, false);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	*size = (u64)dpart.size * dpart.blksz;

	return 0;
}

int mmc_erase_part(u32 dev, int hwpart, const char *part_name, u64 offset,
		   u64 size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	if (!size)
		size = (u64)dpart.size * dpart.blksz;

	return _mmc_erase(mmc, (u64)dpart.start * dpart.blksz + offset,
			  (u64)dpart.size * dpart.blksz, size);
}

int mmc_erase_env_part(u32 dev, int hwpart, const char *part_name, u64 size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	if (!size)
		size = (u64)dpart.size * dpart.blksz;

	return _mmc_erase_env(mmc, (u64)dpart.start * dpart.blksz, size);
}
#endif /* CONFIG_PARTITIONS */

static int adjust_gpt(struct mmc *mmc, void *data, size_t size)
{
	u32 crc32_backup = 0, calc_crc32;
	gpt_header gpt;
	u64 lastlba;

	if (size < (GPT_PRIMARY_PARTITION_TABLE_LBA + 1) * mmc->read_bl_len)
		return -EINVAL;

	memcpy(&gpt, data + GPT_PRIMARY_PARTITION_TABLE_LBA * mmc->read_bl_len,
	       sizeof(gpt));

	/* Check the GPT header signature */
	if (le64_to_cpu(gpt.signature) != GPT_HEADER_SIGNATURE_UBOOT)
		return -EINVAL;

	/* Check the GUID Partition Table CRC */
	memcpy(&crc32_backup, &gpt.header_crc32, sizeof(crc32_backup));
	memset(&gpt.header_crc32, 0, sizeof(gpt.header_crc32));

	calc_crc32 = crc32(0, (const unsigned char *)&gpt,
			   le32_to_cpu(gpt.header_size));

	memcpy(&gpt.header_crc32, &crc32_backup, sizeof(crc32_backup));

	if (calc_crc32 != le32_to_cpu(crc32_backup))
		return -EINVAL;

	/* Check that the my_lba entry points to the LBA that contains the GPT */
	if (le64_to_cpu(gpt.my_lba) != GPT_PRIMARY_PARTITION_TABLE_LBA)
		return -EINVAL;

	/* Check if the first_usable_lba is within the disk. */
	lastlba = (u64)mmc_get_blk_desc(mmc)->lba;
	if (le64_to_cpu(gpt.first_usable_lba) > lastlba)
		return -EINVAL;

	/* Check if the last_usable_lba is within the disk. */
	if (le64_to_cpu(gpt.last_usable_lba) < lastlba)
		return 0;

	/* Prompt to the user */
	printf("Last usable LBA in GPT adjusted to 0x%llx (original 0x%llx)\n",
	       lastlba, le64_to_cpu(gpt.last_usable_lba));

	/* Adjust the last_usable_lba */
	gpt.last_usable_lba = cpu_to_le64(lastlba - 1);

	/* Recalculate the CRC */
	memset(&gpt.header_crc32, 0, sizeof(gpt.header_crc32));

	calc_crc32 = crc32(0, (const unsigned char *)&gpt,
			   le32_to_cpu(gpt.header_size));

	memcpy(&gpt.header_crc32, &calc_crc32, sizeof(calc_crc32));

	/* Write-back changes */
	memcpy(data + GPT_PRIMARY_PARTITION_TABLE_LBA * mmc->read_bl_len, &gpt,
	       sizeof(gpt));

	return 0;
}

int mmc_write_gpt(u32 dev, int hwpart, size_t max_size, const void *data,
		  size_t size)
{
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, true);
	if (!mmc)
		return -ENODEV;

	adjust_gpt(mmc, (void *)data, size);

	ret = _mmc_write(mmc, 0, max_size, data, size, true);
	if (ret)
		return ret;

#ifdef CONFIG_PARTITIONS
	part_init(mmc_get_blk_desc(mmc));
#endif

	return 0;
}

void mmc_setup_boot_options(u32 dev)
{
	struct mmc *mmc;
	int ret, rstbw;

	if (!IS_ENABLED(CONFIG_SUPPORT_EMMC_BOOT))
		return;

	mmc = _mmc_get_dev(dev, 0, true);
	if (!mmc)
		return;

	if (!IS_MMC(mmc))
		return;

	if (IS_ENABLED(CONFIG_MTK_MMC_BOOT_BUS_WIDTH_1B))
		rstbw = EXT_CSD_BUS_WIDTH_1;
	else
		rstbw = EXT_CSD_BUS_WIDTH_8;

	ret = mmc_set_boot_bus_width(mmc, rstbw, 0, 0);
	if (ret) {
		cprintln(ERROR,
			 "*** Failed to set BOOT_BUS_WIDTH, err = %d ***", ret);
	}

	ret = mmc_set_part_conf(mmc, 1, 1, 0);
	if (ret) {
		cprintln(ERROR, "*** Failed to set PART_CONF, err = %d ***",
			 ret);
	}

	ret = mmc_set_rst_n_function(mmc, 1);
	if (ret) {
		cprintln(ERROR,
			 "*** Failed to set RST_N_FUNCTION, err = %d ***", ret);
	}
}

int mmc_is_sd(u32 dev)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, 0, true);
	if (!mmc)
		return -ENODEV;

	return !!IS_SD(mmc);
}

static int _boot_from_mmc(struct mmc *mmc, u64 offset)
{
	ulong data_load_addr = CONFIG_SYS_LOAD_ADDR;
	u32 size;
	int ret;

	ret = _mmc_read(mmc, offset, (void *)data_load_addr, mmc->read_bl_len);
	if (ret)
		return ret;

	switch (genimg_get_format((const void *)data_load_addr)) {
#if defined(CONFIG_LEGACY_IMAGE_FORMAT) && !defined(CONFIG_MTK_DUAL_BOOT)
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
		return -ENOENT;
	}

	ret = _mmc_read(mmc, offset, (void *)data_load_addr, size);
	if (ret)
		return ret;

	return boot_from_mem(data_load_addr);
}

static int _boot_from_mmc_partition(struct mmc *mmc, const char *part_name)
{
	struct disk_partition dpart;
	u64 offset;
	int ret;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	offset = (u64)dpart.start * dpart.blksz;

	return _boot_from_mmc(mmc, offset);
}

int boot_from_mmc_partition(u32 dev, u32 hwpart, const char *part_name)
{
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, hwpart, false);
	if (!mmc)
		return -ENODEV;

	ret = _boot_from_mmc_partition(mmc, part_name);
	if (ret == -ENOENT) {
		printf("Error: no image found in MMC device %u partition '%s'\n",
		       dev, part_name);
	}

	return ret;
}

static int mmc_image_read(struct image_read_priv *rpriv, void *buff, u64 addr,
			   size_t size)
{
	struct mmc_image_read_priv *priv =
		container_of(rpriv, struct mmc_image_read_priv, p);
	struct disk_partition dpart;
	u64 part_size;
	int ret;

	ret = _mmc_find_part(priv->mmc, priv->part, &dpart);
	if (ret)
		return ret;

	part_size = (u64)dpart.size * dpart.blksz;

	if (addr + size > part_size)
		return -EINVAL;

	return _mmc_read(priv->mmc, (u64)dpart.start * dpart.blksz + addr, buff,
			 size);
}

static int mmc_boot_verify(struct mmc *mmc, const struct dual_boot_slot *slot,
			   ulong loadaddr)
{
	struct fit_hashes kernel_hashes, rootfs_hashes;
	struct mmc_image_read_priv read_priv;
	size_t kernel_size, rootfs_size;
	void *kernel_data, *rootfs_data;
	bool ret;

	read_priv.mmc = mmc;

	read_priv.p.page_size = MMC_MAX_BLOCK_LEN;
	read_priv.p.block_size = 0;
	read_priv.p.read = mmc_image_read;

	/* Verify kernel first */
	read_priv.part = slot->kernel;
	kernel_data = (void *)loadaddr;
	ret = read_verify_kernel(&read_priv.p, kernel_data, 0, 0,
				 &kernel_size, &kernel_hashes);
	if (!ret) {
		printf("Error: kernel verification failed\n");
		return -EBADMSG;
	}

	/* Verify rootfs, requiring valid kernel FIT image */
	read_priv.part = slot->rootfs;
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

static int mmc_boot_slot(struct mmc *mmc, uint32_t slot)
{
	ulong data_load_addr = CONFIG_SYS_LOAD_ADDR;
	int ret;

	printf("Trying to boot from image slot %u\n", slot);

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_IMAGE_ROOTFS_VERIFY)) {
		ret = mmc_boot_verify(mmc, &mmc_boot_slots[slot],
				      data_load_addr);
		if (ret) {
			printf("Firmware integrity verification failed\n");
			return ret;
		}

		printf("Firmware integrity verification passed\n");
		return boot_from_mem(data_load_addr);
	}

	return _boot_from_mmc_partition(mmc, mmc_boot_slots[slot].kernel);
}

static int mmc_fill_partuuid(struct mmc *mmc, const char *part_name, char *var)
{
	struct disk_partition dpart;
	int ret;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return -ENODEV;

	debug("part '%s' uuid = %s\n", part_name, dpart.uuid);

	snprintf(var, BOOT_PARAM_STR_MAX_LEN, "PARTUUID=%s", dpart.uuid);

	return 0;
}

static int mmc_set_bootarg_part(struct mmc *mmc, const char *part_name,
				const char *argname, char *argval)
{
	int ret;

	ret = mmc_fill_partuuid(mmc, part_name, argval);
	if (ret) {
		panic("Error: failed to find part named '%s'\n", part_name);
		return ret;
	}

	return bootargs_set(argname, argval);
}

static int mmc_set_bootargs(struct mmc *mmc)
{
	const char *env_part_name = ofnode_conf_read_str("u-boot,mmc-env-partition");
	u32 slot;
	int ret;

	bootargs_reset();

	ret = bootargs_set("boot_param.no_split_rootfs_data", NULL);
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_RESERVE_ROOTFS_DATA)) {
		ret = bootargs_set("boot_param.reserve_rootfs_data", NULL);
		if (ret)
			return ret;
	}

	slot = dual_boot_get_current_slot();

	ret = mmc_set_bootarg_part(mmc, mmc_boot_slots[slot].kernel,
				   "boot_param.boot_kernel_part",
				   boot_kernel_part);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, mmc_boot_slots[slot].rootfs,
				   "boot_param.boot_rootfs_part",
				   boot_rootfs_part);
	if (ret)
		return ret;

	slot = dual_boot_get_next_slot();

	ret = mmc_set_bootarg_part(mmc, mmc_boot_slots[slot].kernel,
				   "boot_param.upgrade_kernel_part",
				   upgrade_kernel_part);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, mmc_boot_slots[slot].rootfs,
				   "boot_param.upgrade_rootfs_part",
				   upgrade_rootfs_part);
	if (ret)
		return ret;

	if (env_part_name) {
		ret = mmc_set_bootarg_part(mmc, env_part_name,
					   "boot_param.env_part", env_part);
		if (ret)
			return ret;
	}

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
	ret = mmc_set_bootarg_part(mmc, CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME,
					"boot_param.rootfs_data_part",
					rootfs_data_part);
	if (ret)
		return ret;
#endif

	return bootargs_set("root", boot_rootfs_part);
}

static int mmc_dual_boot(u32 dev)
{
	struct mmc *mmc;
	u32 slot;
	int ret;

	mmc = _mmc_get_dev(dev, 0, false);
	if (!mmc)
		return -ENODEV;

	slot = dual_boot_get_current_slot();

	if (dual_boot_is_slot_invalid(slot)) {
		printf("Image slot %u was marked invalid.\n", slot);
	} else {
		mmc_set_bootargs(mmc);

		ret = mmc_boot_slot(mmc, slot);

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

		mmc_set_bootargs(mmc);

		ret = mmc_boot_slot(mmc, slot);

		printf("Failed to boot from next image slot, error %d\n", ret);

		dual_boot_set_slot_invalid(slot, true, true);
	}

	return ret;
}

int mmc_boot_image(u32 dev)
{
	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT))
		return mmc_dual_boot(dev);

	return boot_from_mmc_partition(dev, 0, PART_KERNEL_NAME);
}

int mmc_upgrade_image(u32 dev, const void *data, size_t size)
{
	const char *kernel_part, *rootfs_part;
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	loff_t rootfs_data_offs;
	u32 slot;
	int ret;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT)) {
		slot = dual_boot_get_next_slot();
		printf("Upgrading image slot %u ...\n", slot);

		kernel_part = mmc_boot_slots[slot].kernel;
		rootfs_part = mmc_boot_slots[slot].rootfs;
	} else {
		kernel_part = PART_KERNEL_NAME;
		rootfs_part = PART_ROOTFS_NAME;
	}

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	ret = mmc_write_part(dev, 0, rootfs_part, rootfs_data, rootfs_size, true);
	if (ret)
		return ret;

	/* Mark rootfs_data unavailable */
	rootfs_data_offs = (rootfs_size + ROOTDEV_OVERLAY_ALIGN - 1) &
			   (~(ROOTDEV_OVERLAY_ALIGN - 1));
	mmc_erase_part(dev, 0, rootfs_part, rootfs_data_offs, SZ_512K);

	ret = mmc_write_part(dev, 0, kernel_part, kernel_data, kernel_size, true);
	if (ret)
		return ret;

#ifdef CONFIG_MTK_DUAL_BOOT
	ret = dual_boot_set_slot_invalid(slot, false, false);
	if (ret)
		printf("Error: failed to set new image slot valid in env\n");

	ret = dual_boot_set_current_slot(slot);
	if (ret)
		printf("Error: failed to save new image slot to env\n");
#endif /* CONFIG_MTK_DUAL_BOOT */

	return ret;
}
