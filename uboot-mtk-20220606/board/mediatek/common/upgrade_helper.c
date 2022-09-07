// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data upgrading helper
 */

#include <errno.h>
#include <blk.h>
#include <mmc.h>
#include <part.h>
#include <part_efi.h>
#include <spi.h>
#include <linux/bitops.h>
#include <linux/sizes.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <u-boot/crc.h>

#include <dm/device-internal.h>

#include "load_data.h"
#include "upgrade_helper.h"
#include "colored_print.h"

/* 16K for max NAND page size + oob size */
static u8 scratch_buffer[SZ_16K];

static int check_data_size(u64 total_size, u64 offset, size_t max_size,
			   size_t size, bool write)
{
	if (offset + size > total_size) {
		printf("\n");
		cprintln(ERROR, "*** Data %s pasts flash size! ***",
			 write ? "write" : "read");
		goto abort;
	}

	if (!max_size || size <= max_size)
		return 0;

	printf("\n");
	cprintln(ERROR, "*** Data is too large (0x%zx > 0x%zx)! ***", size,
		 max_size);

abort:
	cprintln(ERROR, "*** Operation Aborted! ***");

	return -EINVAL;
}

static bool verify_data(const u8 *orig, const u8 *rdback, u64 offset,
			size_t size)
{
	bool passed = true;
	size_t i;

	for (i = 0; i < size; i++) {
		if (orig[i] != rdback[i]) {
			passed = false;
			break;
		}
	}

	if (passed)
		return true;

	printf("Fail\n");
	cprintln(ERROR, "*** Verification failed at 0x%llx! ***",
		 offset + i);
	cprintln(ERROR, "*** Expected 0x%02x, got 0x%02x ***", orig[i],
		 rdback[i]);
	cprintln(ERROR, "*** Data is damaged, please retry! ***");

	return false;
}

#ifdef CONFIG_MMC
struct mmc *_mmc_get_dev(u32 dev, int part, bool write)
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
	blkcache_invalidate(bd->if_type, bd->devnum);
#endif

	if (write && mmc_getwp(mmc)) {
		cprintln(ERROR, "*** MMC device %u is write-protected! ***",
			 dev);
		return NULL;
	}

	if (part >= 0 && mmc->part_config != MMCPART_NOAVAILABLE) {
		ret = blk_select_hwpart_devnum(IF_TYPE_MMC, dev, part);
		if (ret || mmc_get_blk_desc(mmc)->hwpart != part) {
			cprintln(ERROR,
				 "*** Failed to switch to MMC device %u part %u! ***",
				 dev, part);
			return NULL;
		}
	}

	return mmc;
}

int _mmc_write(struct mmc *mmc, u64 offset, size_t max_size, const void *data,
	       size_t size, bool verify)
{
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
		chksz = min(sizeof(scratch_buffer), size_left);
		blks = (chksz + mmc->read_bl_len - 1) / mmc->read_bl_len;

		n = blk_dread(mmc_get_blk_desc(mmc),
			      (offset + verify_offset) / mmc->read_bl_len,
			      blks, scratch_buffer);

		if (n != blks) {
			printf("Fail\n");
			cprintln(ERROR, "*** Only 0x%zx read! ***",
				 (size_t)n * mmc->read_bl_len);
			return -EIO;
		}

		if (!verify_data(data + verify_offset, scratch_buffer,
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
			       1, scratch_buffer);
		if (n != 1) {
			printf("Fail\n");
			cprintln(ERROR, "*** Failed to read a block! ***");
			return -EIO;
		}

		memcpy(data, scratch_buffer, chksz);
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
			       1, scratch_buffer);
		if (n != 1) {
			printf("Fail\n");
			cprintln(ERROR, "*** Failed to read a block! ***");
			return -EIO;
		}

		memcpy(data, scratch_buffer, size);
	}

success:
	printf("OK\n");

	return 0;
}

int _mmc_erase(struct mmc *mmc, u64 offset, size_t max_size, u64 size)
{
	u32 start_blk, blks, i, n;

	if (check_data_size(mmc->capacity, offset, max_size, size, true))
		return -EINVAL;

	memset(scratch_buffer, 0, sizeof(mmc->write_bl_len));

	start_blk = offset / mmc->write_bl_len;
	blks = size / mmc->write_bl_len;

	for (i = start_blk; i < start_blk + blks; i++) {
		n = blk_dwrite(mmc_get_blk_desc(mmc), i, 1, scratch_buffer);
		if (n != 1)
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

int mmc_write_generic(u32 dev, int part, u64 offset, size_t max_size,
		      const void *data, size_t size, bool verify)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, part, true);
	if (!mmc)
		return -ENODEV;

	return _mmc_write(mmc, offset, max_size, data, size, verify);
}

int mmc_read_generic(u32 dev, int part, u64 offset, void *data, size_t size)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, part, false);
	if (!mmc)
		return -ENODEV;

	return _mmc_read(mmc, offset, data, size);
}

int mmc_erase_generic(u32 dev, int part, u64 offset, u64 size)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, part, true);
	if (!mmc)
		return -ENODEV;

	return _mmc_erase(mmc, offset, 0, size);
}

int mmc_erase_env_generic(u32 dev, int part, u64 offset, u64 size)
{
	struct mmc *mmc;

	mmc = _mmc_get_dev(dev, part, true);
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

int mmc_write_part(u32 dev, int part, const char *part_name, const void *data,
		   size_t size, bool verify)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, part, true);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	return _mmc_write(mmc, (u64)dpart.start * dpart.blksz,
			  (u64)dpart.size * dpart.blksz, data, size, verify);
}

int mmc_read_part(u32 dev, int part, const char *part_name, void *data,
		  size_t size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, part, false);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, part_name, &dpart);
	if (ret)
		return ret;

	return _mmc_read(mmc, (u64)dpart.start * dpart.blksz, data, size);
}

int mmc_erase_part(u32 dev, int part, const char *part_name, u64 offset,
		   u64 size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, part, true);
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

int mmc_erase_env_part(u32 dev, int part, const char *part_name, u64 size)
{
	struct disk_partition dpart;
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, part, true);
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

int mmc_write_gpt(u32 dev, int part, size_t max_size, const void *data,
		  size_t size)
{
	struct mmc *mmc;
	int ret;

	mmc = _mmc_get_dev(dev, part, true);
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
#endif /* CONFIG_GENERIC_MMC */

#ifdef CONFIG_MTD
static int _mtd_erase_generic(struct mtd_info *mtd, u64 offset, u64 size,
			      const char *name)
{
	struct erase_info ei;
	u64 start, end;
	int ret;

	if (!mtd)
		return -EINVAL;

	if (!size)
		return 0;

	if (offset >= mtd->size) {
		printf("\n");
		cprintln(ERROR, "*** Erase offset pasts flash size! ***");
		return -EINVAL;
	}

	if (offset + size > mtd->size) {
		printf("\n");
		cprintln(ERROR, "*** Erase size pasts flash size! ***");
		return -EINVAL;
	}

	start = offset & (~(u64)mtd->erasesize_mask);
	end = (start + size + mtd->erasesize_mask) &
	      (~(u64)mtd->erasesize_mask);

	printf("Erasing%s%s from 0x%llx to 0x%llx, size 0x%llx ... ",
	       name ? " " : "", name ? name : "", start, end - 1, end - start);

	memset(&ei, 0, sizeof(ei));

	ei.mtd = mtd;
	ei.addr = start;
	ei.len = end - start;

	ret = mtd_erase(mtd, &ei);
	if (ret) {
		printf("Fail\n");
		return ret;
	}

	printf("OK\n");

	return 0;
}

int mtd_erase_generic(struct mtd_info *mtd, u64 offset, u64 size)
{
	return _mtd_erase_generic(mtd, offset, size, NULL);
}

int mtd_write_generic(struct mtd_info *mtd, u64 offset, u64 max_size,
		      const void *data, size_t size, bool verify)
{
	size_t size_left, chksz;
	struct mtd_oob_ops ops;
	u64 verify_offset;
	int ret;

	if (!mtd)
		return -EINVAL;

	if (!size)
		return 0;

	if (check_data_size(mtd->size, offset, max_size, size, true))
		return -EINVAL;

	printf("Writing from 0x%lx to 0x%llx, size 0x%zx ... ", (ulong)data,
	       offset, size);

	memset(&ops, 0, sizeof(ops));

	ops.mode = MTD_OPS_AUTO_OOB;
	ops.datbuf = (void *)data;
	ops.len = size;

	ret = mtd_write_oob(mtd, offset, &ops);
	if (ret) {
		printf("Fail\n");
		return ret;
	}

	printf("OK\n");

	if (!verify)
		return 0;

	printf("Verifying from 0x%llx to 0x%llx, size 0x%zx ... ", offset,
	       offset + size - 1, size);

	size_left = size;
	verify_offset = 0;

	while (size_left) {
		chksz = min(sizeof(scratch_buffer), size_left);

		if (chksz > mtd->writesize)
			chksz &= ~(size_t)(mtd->writesize - 1);

		memset(&ops, 0, sizeof(ops));

		ops.mode = MTD_OPS_AUTO_OOB;
		ops.datbuf = scratch_buffer;
		ops.len = chksz;
		ops.retlen = 0;

		ret = mtd_read_oob(mtd, offset + verify_offset, &ops);
		if (ret < 0 && ret != -EUCLEAN) {
			printf("Fail (ret = %d)\n", ret);
			cprintln(ERROR, "*** Only 0x%zx read! ***",
				 size - size_left + ops.retlen);
			return -EIO;
		}

		if (!verify_data(data + verify_offset, scratch_buffer,
				 offset + verify_offset, chksz))
			return -EIO;

		verify_offset += chksz;
		size_left -= chksz;
	}

	printf("OK\n");

	return 0;
}

int mtd_read_generic(struct mtd_info *mtd, u64 offset, void *data, size_t size)
{
	struct mtd_oob_ops ops;
	int ret;

	if (!mtd)
		return -EINVAL;

	if (!size)
		return 0;

	if (check_data_size(mtd->size, offset, 0, size, false))
		return -EINVAL;

	printf("Reading from 0x%llx to 0x%lx, size 0x%zx ... ", offset,
	       (ulong)data, size);

	memset(&ops, 0, sizeof(ops));

	ops.mode = MTD_OPS_AUTO_OOB;
	ops.datbuf = (void *)data;
	ops.len = size;

	ret = mtd_read_oob(mtd, offset, &ops);
	if (ret < 0 && ret != -EUCLEAN) {
		printf("Fail (ret = %d)\n", ret);
		return ret;
	}

	printf("OK\n");

	return 0;
}

int mtd_erase_env(struct mtd_info *mtd, u64 offset, u64 size)
{
	return _mtd_erase_generic(mtd, offset, size, "environment");
}
#endif

#ifdef CONFIG_DM_SPI_FLASH
struct spi_flash *snor_get_dev(int bus, int cs)
{
	struct udevice *new, *bus_dev;
	int ret;

	/* Remove the old device, otherwise probe will just be a nop */
	ret = spi_find_bus_and_cs(bus, cs, &bus_dev, &new);
	if (!ret)
		device_remove(new, DM_REMOVE_NORMAL);

	ret = spi_flash_probe_bus_cs(bus, cs, &new);
	if (!ret)
		return dev_get_uclass_priv(new);

	cprintln(ERROR,
		 "*** Failed to initialize SPI-NOR at %u:%u (error %d) ***",
		 bus, cs, ret);

	return NULL;
}

static int _snor_erase_generic(struct spi_flash *snor, u32 offset, u32 size,
			       const char *name)
{
	u32 start, end, erasesize_mask;
	int ret;

	if (!snor)
		return -EINVAL;

	if (!size)
		return 0;

	if (offset >= snor->size) {
		printf("\n");
		cprintln(ERROR, "*** Erase offset pasts flash size! ***");
		return -EINVAL;
	}

	if (offset + size > snor->size) {
		printf("\n");
		cprintln(ERROR, "*** Erase size pasts flash size! ***");
		return -EINVAL;
	}

	erasesize_mask = (1 << (ffs(snor->erase_size) - 1)) - 1;

	start = offset & (~erasesize_mask);
	end = (start + size + erasesize_mask) & (~erasesize_mask);

	printf("Erasing%s%s from 0x%x to 0x%x, size 0x%x ... ",
	       name ? " " : "", name ? name : "", start, end - 1, end - start);

	ret = spi_flash_erase(snor, start, end - start);
	if (ret) {
		printf("Fail\n");
		return ret;
	}

	printf("OK\n");

	return 0;
}

int snor_erase_generic(struct spi_flash *snor, u32 offset, u32 size)
{
	return _snor_erase_generic(snor, offset, size, NULL);
}

int snor_write_generic(struct spi_flash *snor, u32 offset, u32 max_size,
		       const void *data, size_t size, bool verify)
{
	size_t size_left, chksz;
	u32 verify_offset;
	int ret;

	if (!snor)
		return -EINVAL;

	if (!size)
		return 0;

	if (check_data_size(snor->size, offset, max_size, size, true))
		return -EINVAL;

	printf("Writing from 0x%lx to 0x%x, size 0x%zx ... ", (ulong)data,
	       offset, size);

	ret = spi_flash_write(snor, offset, size, data);
	if (ret) {
		printf("Fail\n");
		return ret;
	}

	printf("OK\n");

	if (!verify)
		return 0;

	printf("Verifying from 0x%x to 0x%zx, size 0x%zx ... ", offset,
	       offset + size - 1, size);

	size_left = size;
	verify_offset = 0;

	while (size_left) {
		chksz = min(sizeof(scratch_buffer), size_left);

		ret = spi_flash_read(snor, offset + verify_offset, chksz,
				     scratch_buffer);
		if (ret) {
			printf("Fail\n");
			cprintln(ERROR, "*** Failed to read SPI-NOR! ***");
			return -EIO;
		}

		if (!verify_data(data + verify_offset, scratch_buffer,
				 offset + verify_offset, chksz))
			return -EIO;

		verify_offset += chksz;
		size_left -= chksz;
	}

	printf("OK\n");

	return 0;
}

int snor_read_generic(struct spi_flash *snor, u32 offset, void *data,
		      size_t size)
{
	int ret;

	if (!snor)
		return -EINVAL;

	if (!size)
		return 0;

	if (check_data_size(snor->size, offset, 0, size, false))
		return -EINVAL;

	printf("Reading from 0x%x to 0x%lx, size 0x%zx ... ", offset,
	       (ulong)data, size);

	ret = spi_flash_read(snor, offset, size, data);
	if (ret) {
		printf("Fail\n");
		return ret;
	}

	printf("OK\n");

	return 0;
}

int snor_erase_env(struct spi_flash *snor, u32 offset, u32 size)
{
	return _snor_erase_generic(snor, offset, size, "environment");
}
#endif
