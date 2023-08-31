/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <errno.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <malloc.h>
#include <memalign.h>
#include <mtd.h>
#include <search.h>

#if CONFIG_ENV_SIZE_REDUND < CONFIG_ENV_SIZE
#undef CONFIG_ENV_SIZE_REDUND
#define CONFIG_ENV_SIZE_REDUND CONFIG_ENV_SIZE
#endif

#if defined(ENV_IS_EMBEDDED)
env_t *env_ptr = &environment;
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr;
#endif /* ENV_IS_EMBEDDED */

DECLARE_GLOBAL_DATA_PTR;

static int env_mtd_init(void)
{
#if defined(ENV_IS_EMBEDDED)
	int crc1_ok = 0, crc2_ok = 0;
	env_t *tmp_env1;

	tmp_env1 = env_ptr;
	crc1_ok = crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc;

	if (!crc1_ok && !crc2_ok) {
		gd->env_addr	= 0;
		gd->env_valid	= ENV_INVALID;

		return 0;
	} else if (crc1_ok && !crc2_ok) {
		gd->env_valid = ENV_VALID;
	}

	if (gd->env_valid == ENV_VALID)
		env_ptr = tmp_env1;

	gd->env_addr = (ulong)env_ptr->data;

#else /* ENV_IS_EMBEDDED */
	gd->env_addr	= (ulong)&default_environment[0];
	gd->env_valid	= ENV_VALID;
#endif /* ENV_IS_EMBEDDED */

	return 0;
}

static struct mtd_info *env_mtd_get_dev(void)
{
	struct mtd_info *mtd;

	mtd_probe_devices();

	mtd = get_mtd_device_nm(CONFIG_ENV_MTD_NAME);
	if (IS_ERR(mtd) || !mtd) {
		printf("MTD device '%s' not found\n", CONFIG_ENV_MTD_NAME);
		return NULL;
	}

	return mtd;
}

static inline bool mtd_addr_is_block_aligned(struct mtd_info *mtd, u64 addr)
{
	return (addr & mtd->erasesize_mask) == 0;
}

static int mtd_io_skip_bad(struct mtd_info *mtd, bool read, loff_t offset,
			   size_t length, size_t redund, u8 *buffer)
{
	struct mtd_oob_ops io_op = {};
	size_t remaining = length;
	loff_t off, end;
	int ret;

	io_op.mode = MTD_OPS_PLACE_OOB;
	io_op.len = mtd->writesize;
	io_op.datbuf = (void *)buffer;

	/* Search for the first good block after the given offset */
	off = offset;
	end = (off + redund) | (mtd->erasesize - 1);
	while (mtd_block_isbad(mtd, off) && off < end)
		off += mtd->erasesize;

	/* Reached end position */
	if (off >= end)
		return -EIO;

	/* Loop over the pages to do the actual read/write */
	while (remaining) {
		/* Skip the block if it is bad */
		if (mtd_addr_is_block_aligned(mtd, off) &&
		    mtd_block_isbad(mtd, off)) {
			off += mtd->erasesize;
			continue;
		}

		if (read)
			ret = mtd_read_oob(mtd, off, &io_op);
		else
			ret = mtd_write_oob(mtd, off, &io_op);

		if (ret) {
			printf("Failure while %s at offset 0x%llx\n",
			       read ? "reading" : "writing", off);
			break;
		}

		off += io_op.retlen;
		remaining -= io_op.retlen;
		io_op.datbuf += io_op.retlen;
		io_op.oobbuf += io_op.oobretlen;

		/* Reached end position */
		if (off >= end)
			return -EIO;
	}

	return 0;
}

#ifdef CONFIG_CMD_SAVEENV
static int mtd_erase_skip_bad(struct mtd_info *mtd, loff_t offset,
			      size_t length, size_t redund)
{
	struct erase_info erase_op = {};
	loff_t end = (offset + redund) | (mtd->erasesize - 1);
	int ret;

	erase_op.mtd = mtd;
	erase_op.addr = offset;
	erase_op.len = length;

	while (erase_op.len) {
		ret = mtd_erase(mtd, &erase_op);

		/* Abort if its not a bad block error */
		if (ret != -EIO)
			return ret;

		printf("Skipping bad block at 0x%08llx\n", erase_op.fail_addr);

		/* Skip bad block and continue behind it */
		erase_op.len -= erase_op.fail_addr - erase_op.addr;
		erase_op.len -= mtd->erasesize;
		erase_op.addr = erase_op.fail_addr + mtd->erasesize;

		/* Reached end position */
		if (erase_op.addr >= end)
			return -EIO;
	}

	return 0;
}

static int env_mtd_save(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	struct mtd_info *mtd;
	int ret = 0;

	ret = env_export(env_new);
	if (ret)
		return ret;

	mtd = env_mtd_get_dev();
	if (!mtd)
		return 1;

	printf("Erasing on MTD device '%s'... ", mtd->name);

	ret = mtd_erase_skip_bad(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
				 CONFIG_ENV_SIZE_REDUND);

	puts(ret ? "FAILED\n" : "OK\n");

	if (ret) {
		put_mtd_device(mtd);
		return 1;
	}

	printf("Writing to MTD device '%s'... ", mtd->name);

	ret = mtd_io_skip_bad(mtd, false, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE,
			      CONFIG_ENV_SIZE_REDUND, (u8 *)env_new);

	puts(ret ? "FAILED\n" : "OK\n");

	put_mtd_device(mtd);

	return !!ret;
}
#endif /* CONFIG_CMD_SAVEENV */

static int readenv(size_t offset, u_char *buf)
{
	struct mtd_info *mtd;
	int ret;

	mtd = env_mtd_get_dev();
	if (!mtd)
		return 1;

	ret = mtd_io_skip_bad(mtd, true, offset, CONFIG_ENV_SIZE,
			      CONFIG_ENV_SIZE_REDUND, buf);

	put_mtd_device(mtd);

	return !!ret;
}

static int env_mtd_load(void)
{
#if !defined(ENV_IS_EMBEDDED)
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, CONFIG_ENV_SIZE);
	int ret;

	ret = readenv(CONFIG_ENV_OFFSET, (u_char *)buf);
	if (ret) {
		env_set_default("readenv() failed", 0);
		return -EIO;
	}

	return env_import(buf, 1, H_EXTERNAL);
#endif /* ! ENV_IS_EMBEDDED */

	return 0;
}

U_BOOT_ENV_LOCATION(mtd) = {
	.location	= ENVL_MTD,
	ENV_NAME("MTD")
	.load		= env_mtd_load,
#if defined(CONFIG_CMD_SAVEENV)
	.save		= env_save_ptr(env_mtd_save),
#endif
	.init		= env_mtd_init,
};
