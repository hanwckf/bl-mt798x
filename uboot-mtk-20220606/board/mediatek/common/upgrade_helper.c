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
#include <mtd.h>
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

int check_data_size(u64 total_size, u64 offset, size_t max_size,
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

bool verify_data(const u8 *orig, const u8 *rdback, u64 offset,
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
void gen_mtd_probe_devices(void)
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
#endif
