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
#include <linux/sizes.h>
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

#include "load_data.h"
#include "image_helper.h"
#include "upgrade_helper.h"
#include "boot_helper.h"
#include "colored_print.h"
#include "verify_helper.h"
#include "mmc_helper.h"
#include "dual_boot.h"
#include "bsp_conf.h"
#include "rootdisk.h"
#include "untar.h"

#define BSPCONF_ALIGNED_SIZE	\
	((sizeof(struct mtk_bsp_conf_data) + SZ_512 - 1) & ~(SZ_512 - 1))

/*
 * Keep this value synchronized with definition in libfstools/rootdisk.c of
 * fstools package
 */
#define ROOTDEV_OVERLAY_ALIGN	(64ULL * 1024ULL)

#define GPT_PRIMARY_PARTITION_ENTRY_LBA 2ULL

#define PART_PRODUCTION_NAME	"production"

struct mmc_image_read_priv {
	struct image_read_priv p;
	struct mmc *mmc;
	const char *part;
};

struct dual_boot_mmc_priv {
	struct dual_boot_priv db;
	struct mmc *mmc;
};

static char boot_kernel_part[BOOT_PARAM_STR_MAX_LEN];
static char boot_rootfs_part[BOOT_PARAM_STR_MAX_LEN];
static char upgrade_kernel_part[BOOT_PARAM_STR_MAX_LEN];
static char upgrade_rootfs_part[BOOT_PARAM_STR_MAX_LEN];
static char env_part[BOOT_PARAM_STR_MAX_LEN];
static char env_size[BOOT_PARAM_STR_MAX_LEN];
static char env_offset[BOOT_PARAM_STR_MAX_LEN];
static char env_redund_offset[BOOT_PARAM_STR_MAX_LEN];

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
static char rootfs_data_part[BOOT_PARAM_STR_MAX_LEN];
#endif

static struct mmc *mmc_image_dev;
static const char *mmc_image_part;

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

static const char *mmc_hwpart_name(struct mmc *mmc)
{
	switch (mmc_get_blk_desc(mmc)->hwpart) {
	case 0:
		if (IS_SD(mmc))
			return "SD";
		return "UDA";

	case 1:
		return "BOOT0";

	case 2:
		return "BOOT1";

	default:
		return "Unknown HW partition";
	}
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

	printf("Writing %s from 0x%lx to 0x%llx, size 0x%zx ... ",
	       mmc_hwpart_name(mmc), (ulong)data, offset, size);

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

	printf("Reading %s from 0x%llx to 0x%lx, size 0x%zx ... ",
	       mmc_hwpart_name(mmc), offset, (ulong)data, size);

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

#ifdef CONFIG_PARTITIONS
int _mmc_find_part(struct mmc *mmc, const char *part_name,
		   struct disk_partition *dpart, bool show_err)
{
	struct blk_desc *mmc_bdev;
	bool found = false;
	u32 i = 1, namelen;
	int ret;

	if (!part_name || !part_name[0])
		return -ENODEV;

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
		if (show_err) {
			cprintln(ERROR, "*** Partition '%s' not found! ***",
				 part_name);
			cprintln(ERROR, "*** Please update GPT first! ***");
		}

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

	ret = _mmc_find_part(mmc, part_name, &dpart, true);
	if (ret)
		return ret;

	printf("Writing 0x%zx bytes to partition '%s'\n", size, part_name);

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

	ret = _mmc_find_part(mmc, part_name, &dpart, true);
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

	ret = _mmc_find_part(mmc, part_name, &dpart, true);
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

	ret = _mmc_find_part(mmc, part_name, &dpart, true);
	if (ret)
		return ret;

	if (!size)
		size = (u64)dpart.size * dpart.blksz;

	return _mmc_erase(mmc, (u64)dpart.start * dpart.blksz + offset,
			  (u64)dpart.size * dpart.blksz, size);
}
#endif /* CONFIG_PARTITIONS */

static int adjust_gpt(struct mmc *mmc, void *data, size_t size)
{
	u64 lastlba, adjusted_last_usable_lba, secondary_lba_num;
	u32 crc32_backup = 0, calc_crc32;
	gpt_header gpt;

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

	/* Check my_lba entry points to the LBA that contains the GPT */
	if (le64_to_cpu(gpt.my_lba) != GPT_PRIMARY_PARTITION_TABLE_LBA)
		return -EINVAL;

	/* Check if the first_usable_lba is within the disk. */
	lastlba = (u64)mmc_get_blk_desc(mmc)->lba;
	if (le64_to_cpu(gpt.first_usable_lba) >= lastlba)
		return -EINVAL;

	/* Adjust gpt.alternate_lba to the last LBA */
	gpt.alternate_lba = cpu_to_le64(lastlba - 1);

	/* Adjust last_usable_lba to the LBA just before 2nd partition table */
	/* Secondary gpt table doesn't need MBR */
	secondary_lba_num = DIV_ROUND_UP(size, mmc->read_bl_len) - 1;
	adjusted_last_usable_lba = lastlba - secondary_lba_num - 1;
	gpt.last_usable_lba = cpu_to_le64(adjusted_last_usable_lba);

	/* Prompt to the user */
	printf("Last usable LBA in GPT adjusted to 0x%llx (original 0x%llx)\n",
	       adjusted_last_usable_lba, le64_to_cpu(gpt.last_usable_lba));

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

static int backup_secondary_gpt(struct mmc *mmc, void *data,
                                size_t size, size_t max_size)
{
	u64 lastlba, backup_offset, secondary_lba;
	u32 calc_crc32, partition_entry_size;
	unsigned long secondary_size;
	gpt_header gpt;
	int ret;

	/* Size of 2nd_gpt_partition_table is one LBA size less than primary */
	secondary_size = size - GPT_PRIMARY_PARTITION_TABLE_LBA * mmc->read_bl_len;
	secondary_lba = DIV_ROUND_UP(secondary_size, mmc->read_bl_len);
	partition_entry_size = secondary_size -
			       (GPT_PRIMARY_PARTITION_TABLE_LBA * mmc->read_bl_len);

	memcpy(&gpt, data + GPT_PRIMARY_PARTITION_TABLE_LBA * mmc->read_bl_len,
	       sizeof(gpt));

	/* Move partition entry to the beginning of data */
	memmove(data, data + GPT_PRIMARY_PARTITION_ENTRY_LBA * mmc->read_bl_len,
		partition_entry_size);

	/* Adjust secondary gpt.my_lba */
	gpt.my_lba = cpu_to_le64(gpt.alternate_lba);

	/* Adjust secondary gpt.partition_entry_lba */
	lastlba = (u64)mmc_get_blk_desc(mmc)->lba;
	gpt.partition_entry_lba = cpu_to_le64(lastlba - secondary_lba);

	/* Recalculate the CRC */
	memset(&gpt.header_crc32, 0, sizeof(gpt.header_crc32));

	calc_crc32 = crc32(0, (const unsigned char *)&gpt,
			   le32_to_cpu(gpt.header_size));

	memcpy(&gpt.header_crc32, &calc_crc32, sizeof(calc_crc32));

	/* Copy gpt_header to data */
	memcpy(data + partition_entry_size, &gpt, sizeof(gpt));
	memset(data + partition_entry_size + sizeof(gpt), 0,
	       mmc->read_bl_len - sizeof(gpt));

	/* Write secondary gpt partition to mmc */
	backup_offset =(lastlba - secondary_lba) * mmc->read_bl_len;
	ret = _mmc_write(mmc, backup_offset, max_size, data,
			 secondary_size, true);
	if (ret)
		return ret;

	printf("*** Secondary GPT has been written to 0x%llx ***",
		backup_offset);

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

	/* BackUp Secondary GPT header & GPT Partition Entry */
	ret = backup_secondary_gpt(mmc, (void *)data, size, max_size);
	if(ret)
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

static int mmc_fill_partuuid(struct mmc *mmc, const char *part_name, char *var)
{
	struct disk_partition dpart;
	int ret;

	ret = _mmc_find_part(mmc, part_name, &dpart, true);
	if (ret)
		return -ENODEV;

	debug("part '%s' uuid = %s\n", part_name, dpart.uuid);

	snprintf(var, BOOT_PARAM_STR_MAX_LEN, "PARTUUID=%s", dpart.uuid);

	return 0;
}

static int mmc_set_fdtarg_part(struct mmc *mmc, const char *part_name,
			       const char *argname, char *argval)
{
	int ret;

	ret = mmc_fill_partuuid(mmc, part_name, argval);
	if (ret) {
		printf("Failed to find part named '%s'\n", part_name);
		return ret;
	}

	return fdtargs_set(argname, argval);
}

static int mmc_set_bootarg_part(struct mmc *mmc, const char *part_name,
				const char *argname, char *argval)
{
	int ret;

	ret = mmc_fill_partuuid(mmc, part_name, argval);
	if (ret) {
		printf("Failed to find part named '%s'\n", part_name);
		return ret;
	}

	return bootargs_set(argname, argval);
}

static int mmc_set_fdtargs_basic(struct mmc *mmc)
{
	struct disk_partition dpart;
	const char *env_part_name;
	u64 env_off;
	ulong len;
	int ret;

#if defined(CONFIG_ENV_MMC_SW_PARTITION)
	env_part_name = CONFIG_ENV_MMC_SW_PARTITION;
#else
	env_part_name = ofnode_conf_read_str("u-boot,mmc-env-partition");
#endif
	if (env_part_name) {
		ret = mmc_set_fdtarg_part(mmc, env_part_name,
					  "mediatek,env-part", env_part);
		if (ret)
			return ret;

		ret = _mmc_find_part(mmc, env_part_name, &dpart, true);
		if (ret)
			return -ENODEV;

		/* round up to dpart.blksz */
		len = DIV_ROUND_UP(CONFIG_ENV_SIZE, dpart.blksz);

		snprintf(env_size, sizeof(env_size), "0x%lx", len * dpart.blksz);

		ret = fdtargs_set("mediatek,env-size", env_size);
		if (ret)
			return ret;

		/* use the top of the partion for the environment */
		env_off = (dpart.size - len) * dpart.blksz;

		snprintf(env_offset, sizeof(env_offset), "0x%llx", env_off);

		ret = fdtargs_set("mediatek,env-offset", env_offset);
		if (ret)
			return ret;

		if (IS_ENABLED(CONFIG_SYS_REDUNDAND_ENVIRONMENT)) {
			/* redundant environment offset */
			env_off = (dpart.size - 2 * len) * dpart.blksz;

			snprintf(env_redund_offset, sizeof(env_redund_offset),
				 "0x%llx", env_off);

			ret = fdtargs_set("mediatek,env-redund-offset",
					  env_redund_offset);
			if (ret)
				return ret;
		}
	}

	return 0;
}

static int _boot_from_mmc(struct mmc *mmc, u64 offset, bool do_boot)
{
	ulong data_load_addr = get_load_addr();
	u32 size, itb_size;
	int ret;

	ret = _mmc_read(mmc, offset, (void *)data_load_addr, mmc->read_bl_len);
	if (ret)
		return ret;

	switch (genimg_get_format((const void *)data_load_addr)) {
#if defined(CONFIG_LEGACY_IMAGE_FORMAT) && !defined(CONFIG_MTK_DUAL_BOOT)
	case IMAGE_FORMAT_LEGACY:
		size = image_get_image_size((const void *)data_load_addr);
		if (size <= mmc->read_bl_len)
			break;

		ret = _mmc_read(mmc, offset + mmc->read_bl_len,
				(void *)(data_load_addr + mmc->read_bl_len),
				size - mmc->read_bl_len);
		if (ret)
			return ret;

		break;
#endif
#if defined(CONFIG_FIT)
	case IMAGE_FORMAT_FIT:
		size = fit_get_size((const void *)data_load_addr);
		if (size <= mmc->read_bl_len)
			break;

		ret = _mmc_read(mmc, offset + mmc->read_bl_len,
				(void *)(data_load_addr + mmc->read_bl_len),
				size - mmc->read_bl_len);
		if (ret)
			return ret;

		itb_size = fit_get_totalsize((const void *)data_load_addr);
		if (itb_size > size) {
			ret = _mmc_read(mmc, offset + size,
					(void *)(data_load_addr + size),
					itb_size - size);
			if (ret)
				return ret;
		}

		break;
#endif
	default:
		return -ENOENT;
	}

	ret = mmc_set_fdtargs_basic(mmc);
	if (ret)
		return ret;

	if (!do_boot)
		return 0;

	return boot_from_mem(data_load_addr);
}

static int _boot_from_mmc_partition(struct mmc *mmc, const char *part_name,
				    bool do_boot)
{
	struct disk_partition dpart;
	struct blk_desc *bdesc;
	u64 offset;
	int ret;

	ret = _mmc_find_part(mmc, part_name, &dpart, false);
	if (ret) {
		bdesc = mmc_get_blk_desc(mmc);

		if (ret == -ENOENT) {
			printf("Error: no image found in MMC device %u partition '%s'\n",
			       bdesc->devnum, part_name);
		} else if (ret == -ENODEV) {
			printf("Error: no partition named '%s' found in MMC device %u\n",
			       part_name, bdesc->devnum);
		}

		return ret;
	}

	mmc_image_dev = mmc;
	mmc_image_part = part_name;

	offset = (u64)dpart.start * dpart.blksz;

	return _boot_from_mmc(mmc, offset, do_boot);
}

int boot_from_mmc_partition(u32 dev, u32 hwpart, const char *part_name,
			    bool do_boot)
{
	struct mmc *mmc;

	mmc_image_dev = NULL;
	mmc_image_part = NULL;

	mmc = _mmc_get_dev(dev, hwpart, false);
	if (!mmc)
		return -ENODEV;

	return _boot_from_mmc_partition(mmc, part_name, do_boot);
}

static int mmc_image_read(struct image_read_priv *rpriv, void *buff, u64 addr,
			   size_t size)
{
	struct mmc_image_read_priv *priv =
		container_of(rpriv, struct mmc_image_read_priv, p);
	struct disk_partition dpart;
	u64 part_size;
	int ret;

	ret = _mmc_find_part(priv->mmc, priv->part, &dpart, true);
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

	read_priv.part = slot->kernel;
	kernel_data = (void *)loadaddr;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ITB_IMAGE)) {
		/* Verify firmware at once */
		ret = read_verify_itb_image(&read_priv.p, kernel_data, 0, 0,
					    &kernel_size);
		if (!ret) {
			printf("Error: itb image verification failed\n");
			return -EBADMSG;
		}

		return 0;
	}

	/* Verify kernel first */
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

static int mmc_boot_slot(struct mmc *mmc, uint32_t slot, bool do_boot)
{
	ulong data_load_addr = get_load_addr();
	int ret;

	printf("Trying to boot from image slot %u\n", slot);

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_IMAGE_ROOTFS_VERIFY) ||
	    IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ITB_IMAGE)) {
		ret = mmc_boot_verify(mmc, &dual_boot_slots[slot],
				      data_load_addr);
		if (ret) {
			printf("Firmware integrity verification failed\n");
			return ret;
		}

		printf("Firmware integrity verification passed\n");

		ret = mmc_set_fdtargs_basic(mmc);
		if (ret)
			return ret;

		if (!do_boot)
			return 0;

		return boot_from_mem(data_load_addr);
	}

	return _boot_from_mmc_partition(mmc, dual_boot_slots[slot].kernel,
					do_boot);
}

static int mmc_set_fdtargs_dual_boot(struct mmc *mmc)
{
	u32 slot;
	int ret;

	slot = dual_boot_get_current_slot();

	ret = mmc_set_fdtarg_part(mmc, dual_boot_slots[slot].kernel,
				  "mediatek,boot-firmware-part",
				  boot_kernel_part);
	if (ret)
		return ret;

	slot = dual_boot_get_next_slot();

	ret = mmc_set_fdtarg_part(mmc, dual_boot_slots[slot].kernel,
				  "mediatek,upgrade-firmware-part",
				  upgrade_kernel_part);
	if (ret)
		return ret;

#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
	ret = fdtargs_set("mediatek,no-split-fitrw", NULL);
	if (ret)
		return ret;

	ret = mmc_set_fdtarg_part(mmc, CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME,
				  "mediatek,rootfs_data-part",
				  rootfs_data_part);
	if (ret)
		return ret;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_RESERVE_ROOTFS_DATA)) {
		ret = fdtargs_set("mediatek,reserve-rootfs_data", NULL);
		if (ret)
			return ret;
	}
#endif

	return 0;
}

static int mmc_set_bootargs(struct mmc *mmc)
{
	const char *env_part_name;
	u32 slot;
	int ret;

#if defined(CONFIG_ENV_MMC_PARTITION)
	env_part_name = CONFIG_ENV_MMC_PARTITION;
#else
	env_part_name = ofnode_conf_read_str("u-boot,mmc-env-partition");
#endif

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_RESERVE_ROOTFS_DATA)) {
		ret = bootargs_set("boot_param.reserve_rootfs_data", NULL);
		if (ret)
			return ret;
	}

	slot = dual_boot_get_current_slot();

	ret = mmc_set_bootarg_part(mmc, dual_boot_slots[slot].kernel,
				   "boot_param.boot_kernel_part",
				   boot_kernel_part);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, dual_boot_slots[slot].rootfs,
				   "boot_param.boot_rootfs_part",
				   boot_rootfs_part);
	if (ret)
		return ret;

	slot = dual_boot_get_next_slot();

	ret = mmc_set_bootarg_part(mmc, dual_boot_slots[slot].kernel,
				   "boot_param.upgrade_kernel_part",
				   upgrade_kernel_part);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, dual_boot_slots[slot].rootfs,
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
	ret = bootargs_set("boot_param.no_split_rootfs_data", NULL);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME,
				   "boot_param.rootfs_data_part",
				   rootfs_data_part);
	if (ret)
		return ret;
#endif

	return bootargs_set("root", boot_rootfs_part);
}

static int mmc_dual_boot_slot(struct dual_boot_priv *priv, u32 slot,
			      bool do_boot)
{
	struct dual_boot_mmc_priv *mmcdb = container_of(priv, struct dual_boot_mmc_priv, db);

	bootargs_reset();
	fdtargs_reset();

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ITB_IMAGE))
		mmc_set_fdtargs_dual_boot(mmcdb->mmc);
	else
		mmc_set_bootargs(mmcdb->mmc);

	return mmc_boot_slot(mmcdb->mmc, slot, do_boot);
}

static int mmc_dual_boot(u32 dev, bool do_boot)
{
	struct dual_boot_mmc_priv mmcdb;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, 0, false);
	if (!mmc)
		return -ENODEV;

	mmcdb.db.boot_slot = mmc_dual_boot_slot;
	mmcdb.mmc = mmc;

	ret = dual_boot(&mmcdb.db, do_boot);

#ifdef CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE
	dual_boot_disable();
	bootargs_reset();
	fdtargs_reset();

	ret = fdtargs_set("mediatek,emergency-boot", NULL);
	if (ret)
		return ret;

#ifndef CONFIG_MTK_DUAL_BOOT_ITB_IMAGE
	ret = mmc_set_bootarg_part(mmc, CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE_ROOTFS_NAME,
				   "root", boot_rootfs_part);
	if (ret)
		return ret;

	printf("Booting emergency image ...\n");

	ret = boot_from_mmc_partition(dev, 0, CONFIG_MTK_DUAL_BOOT_EMERG_IMAGE_KERNEL_NAME,
				      do_boot);
#else
	printf("Booting emergency image ...\n");

	ret = boot_from_mmc_partition(dev, 0, CONFIG_MTK_DUAL_BOOT_EMERG_FIRMWARE_NAME,
				      do_boot);
#endif
#endif

	return ret;
}

int mmc_boot_image(u32 dev, bool do_boot)
{
	const char *part_primary, *part_secondary;
	int ret;

	bootargs_reset();
	fdtargs_reset();

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT))
		return mmc_dual_boot(dev, do_boot);

	if (strcmp(CONFIG_MTK_DEFAULT_FIT_BOOT_CONF, "")) {
		part_primary = PART_PRODUCTION_NAME;
		part_secondary = PART_KERNEL_NAME;
	} else {
		part_primary = PART_KERNEL_NAME;
		part_secondary = PART_PRODUCTION_NAME;
	}

	ret = boot_from_mmc_partition(dev, 0, part_primary, do_boot);
	if (ret == -ENODEV)
		ret = boot_from_mmc_partition(dev, 0, part_secondary, do_boot);

	return ret;
}

void mmc_import_bsp_conf(u32 dev)
{
	struct mtk_bsp_conf_data bspconf1, bspconf2, *bc1 = NULL, *bc2 = NULL;
	struct disk_partition dpart;
	u64 part_offs, part_size;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, 0, false);
	if (!mmc)
		return;

	ret = _mmc_find_part(mmc, MTK_BSP_CONF_NAME, &dpart, false);
	if (ret) {
		printf("Error: no partition named '%s' found in MMC device %u\n",
		       MTK_BSP_CONF_NAME, dev);
		return;
	}

	part_offs = (u64)dpart.start * dpart.blksz;
	part_size = (u64)dpart.size * dpart.blksz;

	ret = _mmc_read(mmc, part_offs, (void *)&bspconf1, sizeof(bspconf1));
	if (!ret)
		bc1 = &bspconf1;

	if (part_size < 2 * BSPCONF_ALIGNED_SIZE) {
		printf("Warning: Partition '%s' is too small for redundancy\n",
		       MTK_BSP_CONF_NAME);
		import_bsp_conf(bc1, NULL);
		return;
	}

	ret = _mmc_read(mmc, part_offs + part_size - BSPCONF_ALIGNED_SIZE,
			(void *)&bspconf2, sizeof(bspconf2));
	if (!ret)
		bc2 = &bspconf2;

	import_bsp_conf(bc1, bc2);
}

int mmc_update_bsp_conf(u32 dev, const void *bspconf, uint32_t index)
{
	u64 part_offs, part_size, write_offset;
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, 0, true);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, MTK_BSP_CONF_NAME, &dpart, false);
	if (ret) {
		printf("Error: no partition named '%s' found in MMC device %u\n",
		       MTK_BSP_CONF_NAME, dev);

		return ret;
	}

	part_offs = (u64)dpart.start * dpart.blksz;
	part_size = (u64)dpart.size * dpart.blksz;

	if (index == 1)
		write_offset = part_offs;
	else
		write_offset = part_offs + part_size - BSPCONF_ALIGNED_SIZE;

	return _mmc_write(mmc, write_offset, BSPCONF_ALIGNED_SIZE, bspconf,
			  sizeof(struct mtk_bsp_conf_data), true);
}

static int mmc_dual_boot_post_upgrade(u32 dev, u32 slot)
{
	int ret;

	ret = dual_boot_set_slot_invalid(slot, false, false);
	if (ret)
		printf("Error: failed to set new image slot valid in env\n");

	ret = dual_boot_set_current_slot(slot);
	if (ret)
		printf("Error: failed to save new image slot to env\n");

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ENABLE_RETRY)) {
		printf("Resetting boot count of image slot %u to 0\n", slot);
		dual_boot_set_boot_count(slot, 0);
	}

	return 0;
}

static int mmc_upgrade_image_itb(u32 dev, const void *data, size_t size,
				 const char *part)
{
	bool do_dual_boot_post = false;
	u32 slot;
	int ret;

	if (!part) {
		if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT)) {
			slot = dual_boot_get_next_slot();
			printf("Upgrading image slot %u ...\n", slot);

			part = dual_boot_slots[slot].kernel;

			do_dual_boot_post = true;
		} else {
			part = PART_PRODUCTION_NAME;
		}
	}

	ret = mmc_write_part(dev, 0, part, data, size, true);
	if (ret)
		return ret;

	if (do_dual_boot_post) {
#ifdef CONFIG_MTK_DUAL_BOOT_SHARED_ROOTFS_DATA
		if (!IS_ENABLED(CONFIG_MTK_DUAL_BOOT_RESERVE_ROOTFS_DATA)) {
			/* Mark shared rootfs_data invalid */
			mmc_erase_part(dev, 0,
				       CONFIG_MTK_DUAL_BOOT_ROOTFS_DATA_NAME,
				       0, SZ_512K);
		}
#endif

		return mmc_dual_boot_post_upgrade(dev, slot);
	}

	return 0;
}

static int mmc_upgrade_image_tar(u32 dev, const void *data, size_t size,
				 const char *kernel_part,
				 const char *rootfs_part)
{
	const void *kernel_data, *rootfs_data;
	size_t kernel_size, rootfs_size;
	bool do_dual_boot_post = false;
	loff_t rootfs_data_offs;
	u32 slot;
	int ret;

	if (!kernel_part) {
		if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT)) {
			slot = dual_boot_get_next_slot();
			printf("Upgrading image slot %u ...\n", slot);

			kernel_part = dual_boot_slots[slot].kernel;
			rootfs_part = dual_boot_slots[slot].rootfs;

			do_dual_boot_post = true;
		} else {
			kernel_part = PART_KERNEL_NAME;
			rootfs_part = PART_ROOTFS_NAME;
		}
	}

	ret = parse_tar_image(data, size, &kernel_data, &kernel_size,
			      &rootfs_data, &rootfs_size);
	if (ret)
		return ret;

	printf("\n");

	ret = mmc_write_part(dev, 0, rootfs_part, rootfs_data, rootfs_size,
			     true);
	if (ret)
		return ret;

	printf("\n");

	/* Mark rootfs_data unavailable */
	rootfs_data_offs = (rootfs_size + ROOTDEV_OVERLAY_ALIGN - 1) &
			   (~(ROOTDEV_OVERLAY_ALIGN - 1));
	mmc_erase_part(dev, 0, rootfs_part, rootfs_data_offs, SZ_512K);

	ret = mmc_write_part(dev, 0, kernel_part, kernel_data, kernel_size,
			      true);
	if (ret)
		return ret;

	if (do_dual_boot_post)
		return mmc_dual_boot_post_upgrade(dev, slot);

	return 0;
}

int mmc_upgrade_image_cust(u32 dev, const void *data, size_t size,
			   const char *kernel_part, const char *rootfs_part)
{
	struct owrt_image_info ii;
	int ret;

	ret = parse_image_ram(data, size, 512, &ii);
	if (ret)
		return ret;

	if (ii.header_type == HEADER_FIT)
		return mmc_upgrade_image_itb(dev, data, size, kernel_part);

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ITB_IMAGE) ||
	    ii.type != IMAGE_TAR || (kernel_part && !rootfs_part))
		return -ENOTSUPP;

	return mmc_upgrade_image_tar(dev, data, size, kernel_part,
				     rootfs_part);
}

int mmc_upgrade_image(u32 dev, const void *data, size_t size)
{
	return mmc_upgrade_image_cust(dev, data, size, NULL, NULL);
}

void mmc_boot_set_defaults(void *fdt)
{
	if (mmc_image_dev && mmc_image_part) {
		rootdisk_set_rootfs_block_part_relax(fdt, mmc_image_part,
						     IS_SD(mmc_image_dev));
	}
}
