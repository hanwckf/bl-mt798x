// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <dm.h>
#include <malloc.h>
#include <mapmem.h>
#include <linux/mtd/mtd.h>
#include <watchdog.h>

#include "mtk-snand.h"

struct mtk_snand_mtd {
	struct udevice *dev;
	struct mtk_snand *snf;
	struct mtk_snand_chip_info cinfo;
	uint8_t *page_cache;
};

static const char snand_mtd_name_prefix[] = "spi-nand";

static u32 snandidx;

static inline struct mtk_snand_mtd *mtd_to_msm(struct mtd_info *mtd)
{
	return mtd->priv;
}

static int mtk_snand_mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);
	u64 start_addr, end_addr;
	int ret;

	/* Do not allow write past end of device */
	if ((instr->addr + instr->len) > mtd->size) {
		pr_debug("%s: attempt to erase beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	start_addr = instr->addr & (~mtd->erasesize_mask);
	end_addr = instr->addr + instr->len;
	if (end_addr & mtd->erasesize_mask) {
		end_addr = (end_addr + mtd->erasesize_mask) &
			   (~mtd->erasesize_mask);
	}

	instr->state = MTD_ERASING;

	while (start_addr < end_addr) {
		WATCHDOG_RESET();

		if (mtk_snand_block_isbad(msm->snf, start_addr)) {
			if (!instr->scrub) {
				instr->fail_addr = start_addr;
				ret = -EIO;
				break;
			}
		}

		ret = mtk_snand_erase_block(msm->snf, start_addr);
		if (ret) {
			instr->fail_addr = start_addr;
			break;
		}

		start_addr += mtd->erasesize;
	}

	if (!ret) {
		instr->state = MTD_ERASE_DONE;
	} else {
		instr->state = MTD_ERASE_FAILED;
		ret = -EIO;
	}

	return ret;
}

static int mtk_snand_mtd_read_data(struct mtk_snand_mtd *msm, uint64_t addr,
				   struct mtd_oob_ops *ops)
{
	struct mtd_info *mtd = dev_get_uclass_priv(msm->dev);
	size_t len, ooblen, maxooblen, chklen;
	uint32_t col, ooboffs;
	uint8_t *datcache, *oobcache;
	bool ecc_failed = false, raw = ops->mode == MTD_OPS_RAW ? true : false;
	int ret, max_bitflips = 0;

	col = addr & mtd->writesize_mask;
	addr &= ~mtd->writesize_mask;
	maxooblen = mtd_oobavail(mtd, ops);
	ooboffs = ops->ooboffs;
	ooblen = ops->ooblen;
	len = ops->len;

	datcache = len ? msm->page_cache : NULL;
	oobcache = ooblen ? msm->page_cache + mtd->writesize : NULL;

	ops->oobretlen = 0;
	ops->retlen = 0;

	while (len || ooblen) {
		WATCHDOG_RESET();

		if (ops->mode == MTD_OPS_AUTO_OOB)
			ret = mtk_snand_read_page_auto_oob(msm->snf, addr,
				datcache, oobcache, maxooblen, NULL, raw);
		else
			ret = mtk_snand_read_page(msm->snf, addr, datcache,
				oobcache, raw);

		if (ret < 0 && ret != -EBADMSG)
			return ret;

		if (ret == -EBADMSG) {
			mtd->ecc_stats.failed++;
			ecc_failed = true;
		} else {
			mtd->ecc_stats.corrected += ret;
			max_bitflips = max_t(int, ret, max_bitflips);
		}

		mtd->ecc_stats.corrected += ret;
		max_bitflips = max_t(int, ret, max_bitflips);

		if (len) {
			/* Move data */
			chklen = mtd->writesize - col;
			if (chklen > len)
				chklen = len;

			memcpy(ops->datbuf + ops->retlen, datcache + col,
			       chklen);
			len -= chklen;
			col = 0; /* (col + chklen) %  */
			ops->retlen += chklen;
		}

		if (ooblen) {
			/* Move oob */
			chklen = maxooblen - ooboffs;
			if (chklen > ooblen)
				chklen = ooblen;

			memcpy(ops->oobbuf + ops->oobretlen, oobcache + ooboffs,
			       chklen);
			ooblen -= chklen;
			ooboffs = 0; /* (ooboffs + chklen) % maxooblen; */
			ops->oobretlen += chklen;
		}

		addr += mtd->writesize;
	}

	return ecc_failed ? -EBADMSG : max_bitflips;
}

static int mtk_snand_mtd_read_oob(struct mtd_info *mtd, loff_t from,
				  struct mtd_oob_ops *ops)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);
	uint32_t maxooblen;

	if (!ops->oobbuf && !ops->datbuf) {
		if (ops->ooblen || ops->len)
			return -EINVAL;

		return 0;
	}

	switch (ops->mode) {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_AUTO_OOB:
	case MTD_OPS_RAW:
		break;
	default:
		pr_debug("%s: unsupported oob mode: %u\n", __func__, ops->mode);
		return -EINVAL;
	}

	maxooblen = mtd_oobavail(mtd, ops);

	/* Do not allow read past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->size) {
		pr_debug("%s: attempt to read beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= maxooblen)) {
		pr_debug("%s: attempt to start read outside oob\n",
			 __func__);
		return -EINVAL;
	}

	if (unlikely(from >= mtd->size ||
	    ops->ooboffs + ops->ooblen > ((mtd->size >> mtd->writesize_shift) -
	    (from >> mtd->writesize_shift)) * maxooblen)) {
		pr_debug("%s: attempt to read beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	return mtk_snand_mtd_read_data(msm, from, ops);
}

static int mtk_snand_mtd_write_data(struct mtk_snand_mtd *msm, uint64_t addr,
				    struct mtd_oob_ops *ops)
{
	struct mtd_info *mtd = dev_get_uclass_priv(msm->dev);
	size_t len, ooblen, maxooblen, chklen, oobwrlen;
	uint32_t col, ooboffs;
	uint8_t *datcache, *oobcache;
	bool raw = ops->mode == MTD_OPS_RAW ? true : false;
	int ret;

	col = addr & mtd->writesize_mask;
	addr &= ~mtd->writesize_mask;
	maxooblen = mtd_oobavail(mtd, ops);
	ooboffs = ops->ooboffs;
	ooblen = ops->ooblen;
	len = ops->len;

	datcache = len ? msm->page_cache : NULL;
	oobcache = ooblen ? msm->page_cache + mtd->writesize : NULL;

	ops->oobretlen = 0;
	ops->retlen = 0;

	while (len || ooblen) {
		WATCHDOG_RESET();

		if (len) {
			/* Move data */
			chklen = mtd->writesize - col;
			if (chklen > len)
				chklen = len;

			memset(datcache, 0xff, col);
			memcpy(datcache + col, ops->datbuf + ops->retlen,
			       chklen);
			memset(datcache + col + chklen, 0xff,
			       mtd->writesize - col - chklen);
			len -= chklen;
			col = 0; /* (col + chklen) %  */
			ops->retlen += chklen;
		}

		oobwrlen = 0;
		if (ooblen) {
			/* Move oob */
			chklen = maxooblen - ooboffs;
			if (chklen > ooblen)
				chklen = ooblen;

			memset(oobcache, 0xff, ooboffs);
			memcpy(oobcache + ooboffs,
			       ops->oobbuf + ops->oobretlen, chklen);
			memset(oobcache + ooboffs + chklen, 0xff,
			       mtd->oobsize - ooboffs - chklen);
			oobwrlen = chklen + ooboffs;
			ooblen -= chklen;
			ooboffs = 0; /* (ooboffs + chklen) % maxooblen; */
			ops->oobretlen += chklen;
		}

		if (ops->mode == MTD_OPS_AUTO_OOB)
			ret = mtk_snand_write_page_auto_oob(msm->snf, addr,
				datcache, oobcache, oobwrlen, NULL, raw);
		else
			ret = mtk_snand_write_page(msm->snf, addr, datcache,
				oobcache, raw);

		if (ret)
			return ret;

		addr += mtd->writesize;
	}

	return 0;
}

static int mtk_snand_mtd_write_oob(struct mtd_info *mtd, loff_t to,
				   struct mtd_oob_ops *ops)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);
	uint32_t maxooblen;

	if (!ops->oobbuf && !ops->datbuf) {
		if (ops->ooblen || ops->len)
			return -EINVAL;

		return 0;
	}

	switch (ops->mode) {
	case MTD_OPS_PLACE_OOB:
	case MTD_OPS_AUTO_OOB:
	case MTD_OPS_RAW:
		break;
	default:
		pr_debug("%s: unsupported oob mode: %u\n", __func__, ops->mode);
		return -EINVAL;
	}

	maxooblen = mtd_oobavail(mtd, ops);

	/* Do not allow write past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->size) {
		pr_debug("%s: attempt to write beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= maxooblen)) {
		pr_debug("%s: attempt to start write outside oob\n",
			 __func__);
		return -EINVAL;
	}

	if (unlikely(to >= mtd->size ||
	    ops->ooboffs + ops->ooblen > ((mtd->size >> mtd->writesize_shift) -
	    (to >> mtd->writesize_shift)) * maxooblen)) {
		pr_debug("%s: attempt to write beyond end of device\n",
			 __func__);
		return -EINVAL;
	}

	return mtk_snand_mtd_write_data(msm, to, ops);
}

static int mtk_snand_mtd_read(struct mtd_info *mtd, loff_t from, size_t len,
			      size_t *retlen, u_char *buf)
{
	struct mtd_oob_ops ops = {
		.mode = MTD_OPS_PLACE_OOB,
		.datbuf = buf,
		.len = len,
	};
	int ret;

	ret = mtk_snand_mtd_read_oob(mtd, from, &ops);

	if (retlen)
		*retlen = ops.retlen;

	return ret;
}

static int mtk_snand_mtd_write(struct mtd_info *mtd, loff_t to, size_t len,
			       size_t *retlen, const u_char *buf)
{
	struct mtd_oob_ops ops = {
		.mode = MTD_OPS_PLACE_OOB,
		.datbuf = (void *)buf,
		.len = len,
	};
	int ret;

	ret = mtk_snand_mtd_write_oob(mtd, to, &ops);

	if (retlen)
		*retlen = ops.retlen;

	return ret;
}

static int mtk_snand_mtd_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);

	return mtk_snand_block_isbad(msm->snf, offs);
}

static int mtk_snand_mtd_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);

	return mtk_snand_block_markbad(msm->snf, offs);
}

static int mtk_snand_ooblayout_ecc(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *oobecc)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);

	if (section)
		return -ERANGE;

	oobecc->offset = msm->cinfo.fdm_size * msm->cinfo.num_sectors;
	oobecc->length = mtd->oobsize - oobecc->offset;

	return 0;
}

static int mtk_snand_ooblayout_free(struct mtd_info *mtd, int section,
				    struct mtd_oob_region *oobfree)
{
	struct mtk_snand_mtd *msm = mtd_to_msm(mtd);

	if (section >= msm->cinfo.num_sectors)
		return -ERANGE;

	oobfree->length = msm->cinfo.fdm_size - 1;
	oobfree->offset = section * msm->cinfo.fdm_size + 1;

	return 0;
}

static const struct mtd_ooblayout_ops mtk_snand_ooblayout = {
	.ecc = mtk_snand_ooblayout_ecc,
	.rfree = mtk_snand_ooblayout_free,
};

static int mtk_snand_mtd_probe(struct udevice *dev)
{
	struct mtk_snand_mtd *msm = dev_get_priv(dev);
	struct mtd_info *mtd = dev_get_uclass_priv(dev);
	struct mtk_snand_platdata mtk_snand_pdata = {};
	size_t namelen;
	fdt_addr_t base;
	int ret;

	base = dev_read_addr_name(dev, "nfi");
	if (base == FDT_ADDR_T_NONE)
		return -EINVAL;
	mtk_snand_pdata.nfi_base = map_sysmem(base, 0);

	base = dev_read_addr_name(dev, "ecc");
	if (base == FDT_ADDR_T_NONE)
		return -EINVAL;
	mtk_snand_pdata.ecc_base = map_sysmem(base, 0);

	mtk_snand_pdata.soc = dev_get_driver_data(dev);
	mtk_snand_pdata.quad_spi = dev_read_bool(dev, "quad-spi");

	ret = mtk_snand_init(NULL, &mtk_snand_pdata, &msm->snf);
	if (ret)
		return ret;

	mtk_snand_get_chip_info(msm->snf, &msm->cinfo);

	msm->page_cache = malloc(msm->cinfo.pagesize + msm->cinfo.sparesize);
	if (!msm->page_cache) {
		printf("%s: failed to allocate memory for page cache\n",
		       __func__);
		ret = -ENOMEM;
		goto errout1;
	}

	namelen = sizeof(snand_mtd_name_prefix) + 12;

	mtd->name = malloc(namelen);
	if (!mtd->name) {
		printf("%s: failed to allocate memory for MTD name\n",
		       __func__);
		ret = -ENOMEM;
		goto errout2;
	}

	msm->dev = dev;

	snprintf(mtd->name, namelen, "%s%u", snand_mtd_name_prefix, snandidx++);

	mtd->priv = msm;
	mtd->dev = dev;
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;

	mtd->size = msm->cinfo.chipsize;
	mtd->erasesize = msm->cinfo.blocksize;
	mtd->writesize = msm->cinfo.pagesize;
	mtd->writebufsize = mtd->writesize;
	mtd->oobsize = msm->cinfo.sparesize;
	mtd->oobavail = msm->cinfo.num_sectors * (msm->cinfo.fdm_size - 1);

	mtd->ooblayout = &mtk_snand_ooblayout;

	mtd->ecc_strength = msm->cinfo.ecc_strength;
	mtd->bitflip_threshold = (mtd->ecc_strength * 3) / 4;
	mtd->ecc_step_size = msm->cinfo.sector_size;

	mtd->_read = mtk_snand_mtd_read;
	mtd->_write = mtk_snand_mtd_write;
	mtd->_erase = mtk_snand_mtd_erase;
	mtd->_read_oob = mtk_snand_mtd_read_oob;
	mtd->_write_oob = mtk_snand_mtd_write_oob;
	mtd->_block_isbad = mtk_snand_mtd_block_isbad;
	mtd->_block_markbad = mtk_snand_mtd_block_markbad;

	ret = add_mtd_device(mtd);
	if (ret) {
		printf("%s: failed to add SPI-NAND MTD device\n", __func__);
		ret = -ENODEV;
		goto errout3;
	}

	printf("SPI-NAND: %s (%lluMB)\n", msm->cinfo.model,
	       msm->cinfo.chipsize >> 20);

	return 0;

errout3:
	free(mtd->name);

errout2:
	free(msm->page_cache);

errout1:
	mtk_snand_cleanup(msm->snf);

	return ret;
}

static const struct udevice_id mtk_snand_ids[] = {
	{ .compatible = "mediatek,mt7622-snand", .data = SNAND_SOC_MT7622 },
	{ .compatible = "mediatek,mt7629-snand", .data = SNAND_SOC_MT7629 },
	{ .compatible = "mediatek,mt7981-snand", .data = SNAND_SOC_MT7981 },
	{ .compatible = "mediatek,mt7986-snand", .data = SNAND_SOC_MT7986 },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(spinand) = {
	.name = "mtk-snand",
	.id = UCLASS_MTD,
	.of_match = mtk_snand_ids,
	.priv_auto = sizeof(struct mtk_snand_mtd),
	.probe = mtk_snand_mtd_probe,
};
