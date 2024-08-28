// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2016-2017 Micron Technology, Inc.
 *
 * Authors:
 *	Peter Pan <peterpandong@micron.com>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */

#define pr_fmt(fmt)	"spi-nand: " fmt

#ifndef __UBOOT__
#include <linux/bitfield.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mtd/casn.h>
#include <linux/mtd/spinand.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi-mem.h>
#else
#include <errno.h>
#include <watchdog.h>
#include <spi.h>
#include <spi-mem.h>
#include <ubi_uboot.h>
#include <dm/device_compat.h>
#include <dm/devres.h>
#include <linux/bitfield.h>
#include <linux/bitops.h>
#include <linux/bug.h>
#include <linux/mtd/casn.h>
#include <linux/mtd/spinand.h>
#include <linux/printk.h>
#endif

struct spinand_plat {
	struct mtd_info *mtd;
};

/* SPI NAND index visible in MTD names */
static int spi_nand_idx;

static void spinand_cache_op_adjust_colum(struct spinand_device *spinand,
					  const struct nand_page_io_req *req,
					  u16 *column)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int shift;

	if (nand->memorg.planes_per_lun < 2)
		return;

	/* The plane number is passed in MSB just above the column address */
	shift = fls(nand->memorg.pagesize);
	*column |= req->pos.plane << shift;
}

static int spinand_read_reg_op(struct spinand_device *spinand, u8 reg, u8 *val)
{
	struct spi_mem_op op = SPINAND_GET_FEATURE_OP(reg,
						      spinand->scratchbuf);
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	*val = *spinand->scratchbuf;
	return 0;
}

static int spinand_write_reg_op(struct spinand_device *spinand, u8 reg, u8 val)
{
	struct spi_mem_op op = SPINAND_SET_FEATURE_OP(reg,
						      spinand->scratchbuf);

	*spinand->scratchbuf = val;
	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_read_status(struct spinand_device *spinand, u8 *status)
{
	return spinand_read_reg_op(spinand, REG_STATUS, status);
}

static int spinand_get_cfg(struct spinand_device *spinand, u8 *cfg)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	if (WARN_ON(spinand->cur_target < 0 ||
		    spinand->cur_target >= nand->memorg.ntargets))
		return -EINVAL;

	*cfg = spinand->cfg_cache[spinand->cur_target];
	return 0;
}

static int spinand_set_cfg(struct spinand_device *spinand, u8 cfg)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	if (WARN_ON(spinand->cur_target < 0 ||
		    spinand->cur_target >= nand->memorg.ntargets))
		return -EINVAL;

	if (spinand->cfg_cache[spinand->cur_target] == cfg)
		return 0;

	ret = spinand_write_reg_op(spinand, REG_CFG, cfg);
	if (ret)
		return ret;

	spinand->cfg_cache[spinand->cur_target] = cfg;
	return 0;
}

/**
 * spinand_upd_cfg() - Update the configuration register
 * @spinand: the spinand device
 * @mask: the mask encoding the bits to update in the config reg
 * @val: the new value to apply
 *
 * Update the configuration register.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_upd_cfg(struct spinand_device *spinand, u8 mask, u8 val)
{
	int ret;
	u8 cfg;

	ret = spinand_get_cfg(spinand, &cfg);
	if (ret)
		return ret;

	cfg &= ~mask;
	cfg |= val;

	return spinand_set_cfg(spinand, cfg);
}

/**
 * spinand_select_target() - Select a specific NAND target/die
 * @spinand: the spinand device
 * @target: the target/die to select
 *
 * Select a new target/die. If chip only has one die, this function is a NOOP.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_select_target(struct spinand_device *spinand, unsigned int target)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	int ret;

	if (WARN_ON(target >= nand->memorg.ntargets))
		return -EINVAL;

	if (spinand->cur_target == target)
		return 0;

	if (nand->memorg.ntargets == 1) {
		spinand->cur_target = target;
		return 0;
	}

	ret = spinand->select_target(spinand, target);
	if (ret)
		return ret;

	spinand->cur_target = target;
	return 0;
}

static int spinand_init_cfg_cache(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	struct udevice *dev = spinand->slave->dev;
	unsigned int target;
	int ret;

	spinand->cfg_cache = devm_kzalloc(dev,
					  sizeof(*spinand->cfg_cache) *
					  nand->memorg.ntargets,
					  GFP_KERNEL);
	if (!spinand->cfg_cache)
		return -ENOMEM;

	for (target = 0; target < nand->memorg.ntargets; target++) {
		ret = spinand_select_target(spinand, target);
		if (ret)
			return ret;

		/*
		 * We use spinand_read_reg_op() instead of spinand_get_cfg()
		 * here to bypass the config cache.
		 */
		ret = spinand_read_reg_op(spinand, REG_CFG,
					  &spinand->cfg_cache[target]);
		if (ret)
			return ret;
	}

	return 0;
}

static int spinand_init_quad_enable(struct spinand_device *spinand)
{
	bool enable = false;

	if (!(spinand->flags & SPINAND_HAS_QE_BIT))
		return 0;

	if (spinand->op_templates.read_cache->data.buswidth == 4 ||
	    spinand->op_templates.write_cache->data.buswidth == 4 ||
	    spinand->op_templates.update_cache->data.buswidth == 4)
		enable = true;

	return spinand_upd_cfg(spinand, CFG_QUAD_ENABLE,
			       enable ? CFG_QUAD_ENABLE : 0);
}

static int spinand_ecc_enable(struct spinand_device *spinand,
			      bool enable)
{
	return spinand_upd_cfg(spinand, CFG_ECC_ENABLE,
			       enable ? CFG_ECC_ENABLE : 0);
}

static int spinand_write_enable_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_WR_EN_DIS_OP(true);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_load_page_op(struct spinand_device *spinand,
				const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, &req->pos);
	struct spi_mem_op op = SPINAND_PAGE_READ_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_read_from_cache_op(struct spinand_device *spinand,
				      const struct nand_page_io_req *req)
{
	struct spi_mem_op op = *spinand->op_templates.read_cache;
	struct nand_device *nand = spinand_to_nand(spinand);
	struct mtd_info *mtd = nanddev_to_mtd(nand);
	struct nand_page_io_req adjreq = *req;
	unsigned int nbytes = 0;
	void *buf = NULL;
	u16 column = 0;
	int ret;

	if (req->datalen) {
		adjreq.datalen = nanddev_page_size(nand);
		adjreq.dataoffs = 0;
		adjreq.databuf.in = spinand->databuf;
		buf = spinand->databuf;
		nbytes = adjreq.datalen;
	}

	if (req->ooblen) {
		adjreq.ooblen = nanddev_per_page_oobsize(nand);
		adjreq.ooboffs = 0;
		adjreq.oobbuf.in = spinand->oobbuf;
		nbytes += nanddev_per_page_oobsize(nand);
		if (!buf) {
			buf = spinand->oobbuf;
			column = nanddev_page_size(nand);
		}
	}

	spinand_cache_op_adjust_colum(spinand, &adjreq, &column);
	op.addr.val = column;

	/*
	 * Some controllers are limited in term of max RX data size. In this
	 * case, just repeat the READ_CACHE operation after updating the
	 * column.
	 */
	while (nbytes) {
		op.data.buf.in = buf;
		op.data.nbytes = nbytes;
		ret = spi_mem_adjust_op_size(spinand->slave, &op);
		if (ret)
			return ret;

		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;
	}

	if (req->datalen)
		memcpy(req->databuf.in, spinand->databuf + req->dataoffs,
		       req->datalen);

	if (req->ooblen) {
		if (req->mode == MTD_OPS_AUTO_OOB)
			mtd_ooblayout_get_databytes(mtd, req->oobbuf.in,
						    spinand->oobbuf,
						    req->ooboffs,
						    req->ooblen);
		else
			memcpy(req->oobbuf.in, spinand->oobbuf + req->ooboffs,
			       req->ooblen);
	}

	return 0;
}

static int spinand_write_to_cache_op(struct spinand_device *spinand,
				     const struct nand_page_io_req *req)
{
	struct spi_mem_op op = *spinand->op_templates.write_cache;
	struct nand_device *nand = spinand_to_nand(spinand);
	struct mtd_info *mtd = nanddev_to_mtd(nand);
	struct nand_page_io_req adjreq = *req;
	unsigned int nbytes = 0;
	void *buf = NULL;
	u16 column = 0;
	int ret;

	/*
	 * Looks like PROGRAM LOAD (AKA write cache) does not necessarily reset
	 * the cache content to 0xFF (depends on vendor implementation), so we
	 * must fill the page cache entirely even if we only want to program
	 * the data portion of the page, otherwise we might corrupt the BBM or
	 * user data previously programmed in OOB area.
	 */
	memset(spinand->databuf, 0xff,
	       nanddev_page_size(nand) +
	       nanddev_per_page_oobsize(nand));

	if (req->datalen) {
		memcpy(spinand->databuf + req->dataoffs, req->databuf.out,
		       req->datalen);
		adjreq.dataoffs = 0;
		adjreq.datalen = nanddev_page_size(nand);
		adjreq.databuf.out = spinand->databuf;
		nbytes = adjreq.datalen;
		buf = spinand->databuf;
	}

	if (req->ooblen) {
		if (req->mode == MTD_OPS_AUTO_OOB)
			mtd_ooblayout_set_databytes(mtd, req->oobbuf.out,
						    spinand->oobbuf,
						    req->ooboffs,
						    req->ooblen);
		else
			memcpy(spinand->oobbuf + req->ooboffs, req->oobbuf.out,
			       req->ooblen);

		adjreq.ooblen = nanddev_per_page_oobsize(nand);
		adjreq.ooboffs = 0;
		nbytes += nanddev_per_page_oobsize(nand);
		if (!buf) {
			buf = spinand->oobbuf;
			column = nanddev_page_size(nand);
		}
	}

	spinand_cache_op_adjust_colum(spinand, &adjreq, &column);

	op = *spinand->op_templates.write_cache;
	op.addr.val = column;

	/*
	 * Some controllers are limited in term of max TX data size. In this
	 * case, split the operation into one LOAD CACHE and one or more
	 * LOAD RANDOM CACHE.
	 */
	while (nbytes) {
		op.data.buf.out = buf;
		op.data.nbytes = nbytes;

		ret = spi_mem_adjust_op_size(spinand->slave, &op);
		if (ret)
			return ret;

		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret)
			return ret;

		buf += op.data.nbytes;
		nbytes -= op.data.nbytes;
		op.addr.val += op.data.nbytes;

		/*
		 * We need to use the RANDOM LOAD CACHE operation if there's
		 * more than one iteration, because the LOAD operation resets
		 * the cache to 0xff.
		 */
		if (nbytes) {
			column = op.addr.val;
			op = *spinand->op_templates.update_cache;
			op.addr.val = column;
		}
	}

	return 0;
}

static int spinand_program_op(struct spinand_device *spinand,
			      const struct nand_page_io_req *req)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int row = nanddev_pos_to_row(nand, &req->pos);
	struct spi_mem_op op = SPINAND_PROG_EXEC_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_erase_op(struct spinand_device *spinand,
			    const struct nand_pos *pos)
{
	struct nand_device *nand = &spinand->base;
	unsigned int row = nanddev_pos_to_row(nand, pos);
	struct spi_mem_op op = SPINAND_BLK_ERASE_OP(row);

	return spi_mem_exec_op(spinand->slave, &op);
}

static int spinand_wait(struct spinand_device *spinand, u8 *s)
{
	unsigned long start, stop;
	u8 status;
	int ret;

	start = get_timer(0);
	stop = 400;
	do {
		ret = spinand_read_status(spinand, &status);
		if (ret)
			return ret;

		if (!(status & STATUS_BUSY))
			goto out;
	} while (get_timer(start) < stop);

	/*
	 * Extra read, just in case the STATUS_READY bit has changed
	 * since our last check
	 */
	ret = spinand_read_status(spinand, &status);
	if (ret)
		return ret;

out:
	if (s)
		*s = status;

	return status & STATUS_BUSY ? -ETIMEDOUT : 0;
}

static int spinand_read_id_op(struct spinand_device *spinand, u8 naddr,
			      u8 ndummy, u8 *buf)
{
	struct spi_mem_op op = SPINAND_READID_OP(naddr, ndummy,
						 spinand->scratchbuf,
						 SPINAND_MAX_ID_LEN);
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (!ret)
		memcpy(buf, spinand->scratchbuf, SPINAND_MAX_ID_LEN);

	return ret;
}

static int spinand_reset_op(struct spinand_device *spinand)
{
	struct spi_mem_op op = SPINAND_RESET_OP;
	int ret;

	ret = spi_mem_exec_op(spinand->slave, &op);
	if (ret)
		return ret;

	return spinand_wait(spinand, NULL);
}

static int spinand_lock_block(struct spinand_device *spinand, u8 lock)
{
	return spinand_write_reg_op(spinand, REG_BLOCK_LOCK, lock);
}

static size_t eccsr_none_op(size_t val, size_t mask) { return val; }
static size_t eccsr_and_op(size_t val, size_t mask) { return val & mask; }
static size_t eccsr_add_op(size_t val, size_t mask) { return val + mask; }
static size_t eccsr_minus_op(size_t val, size_t mask) { return val - mask; }
static size_t eccsr_mul_op(size_t val, size_t mask) { return val * mask; }

static void spinand_read_adv_ecc(struct spinand_device *spinand,
				 struct spi_mem_op *ops, u16 *eccsr,
				 u16 mask, u8 shift,
				 u8 pre_op, u8 pre_mask)
{
	u8 *p = spinand->scratchbuf;

	spi_mem_exec_op(spinand->slave, ops);

	if (likely(mask <= 0xff))
		*eccsr += (*p & mask) >> shift;
	else
		*eccsr += (((*p << 8) | (*p+1)) & mask) >> shift;

	*eccsr = spinand->eccsr_math_op[pre_op](*eccsr, pre_mask);
}

static int spinand_casn_get_ecc_status(struct spinand_device *spinand, u8 status)
{
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	struct CASN_ADVECC *ah = spinand->advecc_high;
	struct CASN_ADVECC *al = spinand->advecc_low;
	u16 eccsr_high = 0;
	u16 eccsr_low = 0;
	u32 eccsr = 0;

	if (al->cmd) {
		spinand_read_adv_ecc(spinand,
				     spinand->advecc_low_ops, &eccsr_low,
				     al->mask, al->shift,
				     al->pre_op, al->pre_mask);
		eccsr += eccsr_low;
	}
	if (ah->cmd) {
		spinand_read_adv_ecc(spinand,
				     spinand->advecc_high_ops, &eccsr_high,
				     ah->mask, ah->shift,
				     ah->pre_op, ah->pre_mask);
		eccsr += eccsr_high << spinand->advecc_low_bitcnt;
	}

	if (eccsr == spinand->advecc_noerr_status)
		return 0;
	else if (eccsr == spinand->advecc_uncor_status)
		return -EBADMSG;
	eccsr = spinand->eccsr_math_op[spinand->advecc_post_op](eccsr, spinand->advecc_post_mask);

	return eccsr > mtd->ecc_strength ? mtd->ecc_strength : eccsr;
}

static int spinand_check_ecc_status(struct spinand_device *spinand, u8 status)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	if (spinand->eccinfo.get_status)
		return spinand->eccinfo.get_status(spinand, status);

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_HAS_BITFLIPS:
		/*
		 * We have no way to know exactly how many bitflips have been
		 * fixed, so let's return the maximum possible value so that
		 * wear-leveling layers move the data immediately.
		 */
		return nand->eccreq.strength;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	default:
		break;
	}

	return -EINVAL;
}

static int spinand_read_page(struct spinand_device *spinand,
			     const struct nand_page_io_req *req,
			     bool ecc_enabled)
{
	u8 status;
	int ret;

	ret = spinand_load_page_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (ret < 0)
		return ret;

	ret = spinand_read_from_cache_op(spinand, req);
	if (ret)
		return ret;

	if (!ecc_enabled)
		return 0;

	return spinand_check_ecc_status(spinand, status);
}

static int spinand_write_page(struct spinand_device *spinand,
			      const struct nand_page_io_req *req)
{
	u8 status;
	int ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_write_to_cache_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_program_op(spinand, req);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_PROG_FAILED))
		ret = -EIO;

	return ret;
}

static int spinand_mtd_read(struct mtd_info *mtd, loff_t from,
			    struct mtd_oob_ops *ops)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	unsigned int max_bitflips = 0;
	struct nand_io_iter iter;
	bool enable_ecc = false;
	bool ecc_failed = false;
	int ret = 0;

	if (ops->mode != MTD_OPS_RAW && spinand->eccinfo.ooblayout)
		enable_ecc = true;

#ifndef __UBOOT__
	mutex_lock(&spinand->lock);
#endif

	nanddev_io_for_each_page(nand, from, ops, &iter) {
		schedule();
		ret = spinand_select_target(spinand, iter.req.pos.target);
		if (ret)
			break;

		ret = spinand_ecc_enable(spinand, enable_ecc);
		if (ret)
			break;

		ret = spinand_read_page(spinand, &iter.req, enable_ecc);
		if (ret < 0 && ret != -EBADMSG)
			break;

		if (ret == -EBADMSG) {
			ecc_failed = true;
			mtd->ecc_stats.failed++;
		} else {
			mtd->ecc_stats.corrected += ret;
			max_bitflips = max_t(unsigned int, max_bitflips, ret);
		}

		ret = 0;
		ops->retlen += iter.req.datalen;
		ops->oobretlen += iter.req.ooblen;
	}

#ifndef __UBOOT__
	mutex_unlock(&spinand->lock);
#endif
	if (ecc_failed && !ret)
		ret = -EBADMSG;

	return ret ? ret : max_bitflips;
}

static int spinand_mtd_write(struct mtd_info *mtd, loff_t to,
			     struct mtd_oob_ops *ops)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_io_iter iter;
	bool enable_ecc = false;
	int ret = 0;

	if (ops->mode != MTD_OPS_RAW && mtd->ooblayout)
		enable_ecc = true;

#ifndef __UBOOT__
	mutex_lock(&spinand->lock);
#endif

	nanddev_io_for_each_page(nand, to, ops, &iter) {
		schedule();
		ret = spinand_select_target(spinand, iter.req.pos.target);
		if (ret)
			break;

		ret = spinand_ecc_enable(spinand, enable_ecc);
		if (ret)
			break;

		ret = spinand_write_page(spinand, &iter.req);
		if (ret)
			break;

		ops->retlen += iter.req.datalen;
		ops->oobretlen += iter.req.ooblen;
	}

#ifndef __UBOOT__
	mutex_unlock(&spinand->lock);
#endif

	return ret;
}

static bool spinand_isbad(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	u8 marker[2] = { };
	struct nand_page_io_req req = {
		.pos = *pos,
		.ooblen = sizeof(marker),
		.ooboffs = 0,
		.oobbuf.in = marker,
		.mode = MTD_OPS_RAW,
	};

	spinand_select_target(spinand, pos->target);
	spinand_read_page(spinand, &req, false);
	if (marker[0] != 0xff || marker[1] != 0xff)
		return true;

	return false;
}

static int spinand_mtd_block_isbad(struct mtd_info *mtd, loff_t offs)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
#ifndef __UBOOT__
	struct spinand_device *spinand = nand_to_spinand(nand);
#endif
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
#ifndef __UBOOT__
	mutex_lock(&spinand->lock);
#endif
	ret = nanddev_isbad(nand, &pos);
#ifndef __UBOOT__
	mutex_unlock(&spinand->lock);
#endif
	return ret;
}

static int spinand_markbad(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	u8 marker[2] = { };
	struct nand_page_io_req req = {
		.pos = *pos,
		.ooboffs = 0,
		.ooblen = sizeof(marker),
		.oobbuf.out = marker,
		.mode = MTD_OPS_RAW,
	};
	int ret;

	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	return spinand_write_page(spinand, &req);
}

static int spinand_mtd_block_markbad(struct mtd_info *mtd, loff_t offs)
{
	struct nand_device *nand = mtd_to_nanddev(mtd);
#ifndef __UBOOT__
	struct spinand_device *spinand = nand_to_spinand(nand);
#endif
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
#ifndef __UBOOT__
	mutex_lock(&spinand->lock);
#endif
	ret = nanddev_markbad(nand, &pos);
#ifndef __UBOOT__
	mutex_unlock(&spinand->lock);
#endif
	return ret;
}

static int spinand_erase(struct nand_device *nand, const struct nand_pos *pos)
{
	struct spinand_device *spinand = nand_to_spinand(nand);
	u8 status;
	int ret;

	ret = spinand_select_target(spinand, pos->target);
	if (ret)
		return ret;

	ret = spinand_write_enable_op(spinand);
	if (ret)
		return ret;

	ret = spinand_erase_op(spinand, pos);
	if (ret)
		return ret;

	ret = spinand_wait(spinand, &status);
	if (!ret && (status & STATUS_ERASE_FAILED))
		ret = -EIO;

	return ret;
}

static int spinand_mtd_erase(struct mtd_info *mtd,
			     struct erase_info *einfo)
{
#ifndef __UBOOT__
	struct spinand_device *spinand = mtd_to_spinand(mtd);
#endif
	int ret;

#ifndef __UBOOT__
	mutex_lock(&spinand->lock);
#endif
	ret = nanddev_mtd_erase(mtd, einfo);
#ifndef __UBOOT__
	mutex_unlock(&spinand->lock);
#endif

	return ret;
}

static int spinand_mtd_block_isreserved(struct mtd_info *mtd, loff_t offs)
{
#ifndef __UBOOT__
	struct spinand_device *spinand = mtd_to_spinand(mtd);
#endif
	struct nand_device *nand = mtd_to_nanddev(mtd);
	struct nand_pos pos;
	int ret;

	nanddev_offs_to_pos(nand, offs, &pos);
#ifndef __UBOOT__
	mutex_lock(&spinand->lock);
#endif
	ret = nanddev_isreserved(nand, &pos);
#ifndef __UBOOT__
	mutex_unlock(&spinand->lock);
#endif

	return ret;
}

static const struct nand_ops spinand_ops = {
	.erase = spinand_erase,
	.markbad = spinand_markbad,
	.isbad = spinand_isbad,
};

static const struct spinand_manufacturer *spinand_manufacturers[] = {
	&dosilicon_spinand_manufacturer,
	&etron_spinand_manufacturer,
	&fmsh_spinand_manufacturer,
	&foresee_spinand_manufacturer,
	&gigadevice_spinand_manufacturer,
	&gsto_spinand_manufacturer,
	&macronix_spinand_manufacturer,
	&micron_spinand_manufacturer,
	&paragon_spinand_manufacturer,
	&toshiba_spinand_manufacturer,
	&winbond_spinand_manufacturer,
	&esmt_c8_spinand_manufacturer,
	&xtx_spinand_manufacturer,
};

static int spinand_manufacturer_match(struct spinand_device *spinand,
				      enum spinand_readid_method rdid_method)
{
	u8 *id = spinand->id.data;
	unsigned int i;
	int ret;

	for (i = 0; i < ARRAY_SIZE(spinand_manufacturers); i++) {
		const struct spinand_manufacturer *manufacturer =
			spinand_manufacturers[i];

		if (id[0] != manufacturer->id)
			continue;

		ret = spinand_match_and_init(spinand,
					     manufacturer->chips,
					     manufacturer->nchips,
					     rdid_method);
		if (ret < 0)
			continue;

		spinand->manufacturer = manufacturer;
		return 0;
	}
	return -ENOTSUPP;
}

static u16 nanddev_crc16(u16 crc, u8 const *p, size_t len)
{
	int i;
	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

/* Sanitize ONFI strings so we can safely print them */
static void sanitize_string(char *s, size_t len)
{
	ssize_t i;

	/* Null terminate */
	s[len - 1] = 0;

	/* Remove non printable chars */
	for (i = 0; i < len - 1; i++) {
		if (s[i] < ' ' || s[i] > 127)
			s[i] = '?';
	}

	/* Remove trailing spaces */
	strim(s);
}

/*
 * Recover data with bit-wise majority
 */
static void nanddev_bit_wise_majority(const void **srcbufs,
				   unsigned int nsrcbufs,
				   void *dstbuf,
				   unsigned int bufsize)
{
	int i, j, k;

	for (i = 0; i < bufsize; i++) {
		u8 val = 0;

		for (j = 0; j < 8; j++) {
			unsigned int cnt = 0;

			for (k = 0; k < nsrcbufs; k++) {
				const u8 *srcbuf = srcbufs[k];

				if (srcbuf[i] & BIT(j))
					cnt++;
			}

			if (cnt > nsrcbufs / 2)
				val |= BIT(j);
		}

		((u8 *)dstbuf)[i] = val;
	}
}

static int spinand_check_casn_validity(struct spinand_device *spinand,
				       struct nand_casn *casn)
{
	struct udevice *dev = spinand->slave->dev;

	if (be32_to_cpu(casn->bits_per_cell) != 1) {
		dev_err(dev, "[CASN] bits-per-cell must be 1\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->bytes_per_page)) {
	case 2048:
	case 4096:
		break;
	default:
		dev_err(dev, "[CASN] page size must be 2048/4096\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->spare_bytes_per_page)) {
	case 64:
	case 96:
	case 128:
	case 256:
		break;
	default:
		dev_err(dev, "[CASN] spare size must be 64/128/256\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->pages_per_block)) {
	case 64:
	case 128:
		break;
	default:
		dev_err(dev, "[CASN] pages_per_block must be 64/128\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->blocks_per_lun)) {
	case 1024:
		if (be32_to_cpu(casn->max_bb_per_lun) != 20) {
			dev_err(dev, "[CASN] max_bb_per_lun must be 20 when blocks_per_lun is 1024\n");
			return -EINVAL;
		}
		break;
	case 2048:
		if (be32_to_cpu(casn->max_bb_per_lun) != 40) {
			dev_err(dev, "[CASN] max_bb_per_lun must be 40 when blocks_per_lun is 2048\n");
			return -EINVAL;
		}
		break;
	case 4096:
		if (be32_to_cpu(casn->max_bb_per_lun) != 80) {
			dev_err(dev, "[CASN] max_bb_per_lun must be 80 when blocks_per_lun is 4096\n");
			return -EINVAL;
		}
		break;
	default:
		dev_err(dev, "[CASN] blocks_per_lun must be 1024/2048/4096\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->planes_per_lun)) {
	case 1:
	case 2:
		break;
	default:
		dev_err(dev, "[CASN] planes_per_lun must be 1/2\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->luns_per_target)) {
	case 1:
	case 2:
		break;
	default:
		dev_err(dev, "[CASN] luns_per_target must be 1/2\n");
		return -EINVAL;
	}

	switch (be32_to_cpu(casn->total_target)) {
	case 1:
	case 2:
		break;
	default:
		dev_err(dev, "[CASN] ntargets must be 1/2\n");
		return -EINVAL;
	}

	if (casn->casn_oob.layout_type != OOB_CONTINUOUS &&
	    casn->casn_oob.layout_type != OOB_DISCRETE) {
		dev_err(dev, "[CASN] OOB layout type isn't correct.\n");
		return -EINVAL;
	}

	if (casn->ecc_status_high.status_nbytes > 2 ||
	    casn->ecc_status_low.status_nbytes > 2) {
		dev_err(dev, "[CASN] ADVECC status nbytes must be no more than 2\n");
		return -EINVAL;
	}

	return 0;
}

static int spinand_check_casn(struct spinand_device *spinand,
			struct nand_casn *casn, unsigned int *sel)
{
	struct udevice *dev = spinand->slave->dev;
	uint16_t crc = be16_to_cpu(casn->crc);
	uint16_t crc_compute;
	int ret = 0;
	int i;

	/* There are 3 copies of CASN Pages V1. Choose one avabilable copy
	 * first. If none of the copies is available, try to recover.
	 */
	for (i = 0; i < CASN_PAGE_V1_COPIES; i++) {
		if (be32_to_cpu(casn[i].signature) != CASN_SIGNATURE) {
			ret = -EINVAL;
			continue;
		}
		crc_compute = nanddev_crc16(CASN_CRC_BASE, (u8 *)(casn + i),
					    SPINAND_CASN_V1_CRC_OFS);
		dev_dbg(dev, "CASN COPY %d CRC read: 0x%x, compute: 0x%x\n",
			i, crc, crc_compute);
		if (crc != crc_compute) {
			ret = -EBADMSG;
			continue;
		}
		ret = spinand_check_casn_validity(spinand, casn + i);
		if (ret < 0)
			continue;
		*sel = i;
		break;
	}

	if (i == CASN_PAGE_V1_COPIES && ret == -EBADMSG) {
		const void *srcbufs[CASN_PAGE_V1_COPIES];
		int j;

		for (j = 0; j < CASN_PAGE_V1_COPIES; j++)
			srcbufs[j] = casn + j;
		dev_info(dev, "Couldn't find a valid CASN page, try bitwise majority to recover it\n");
		nanddev_bit_wise_majority(srcbufs, CASN_PAGE_V1_COPIES, casn,
					  sizeof(*casn));
		crc_compute = nanddev_crc16(CASN_CRC_BASE, (uint8_t *)casn,
					    SPINAND_CASN_V1_CRC_OFS);
		if (crc_compute != crc) {
			dev_err(dev, "CASN page recovery failed, aborting\n");
			return -EBADMSG;
		}
		ret = spinand_check_casn_validity(spinand, casn + i);
		if (ret < 0)
			return ret;
		dev_info(dev, "CASN page recovery succeeded\n");
		*sel = 0;
	}

	return ret;
}

static int spinand_casn_detect(struct spinand_device *spinand,
			       struct nand_casn *casn, unsigned int *sel)
{
	struct udevice *dev = spinand->slave->dev;
	uint8_t casn_offset[3] = {0x0, 0x1, 0x4};
	struct nand_page_io_req req;
	struct spi_mem_op op;
	struct nand_pos pos;
	int check_ret = 0;
	uint8_t status;
	int final_ret;
	int ret = 0;
	u8 cfg_reg;
	int i;

	ret = spinand_read_reg_op(spinand, REG_CFG, &cfg_reg);
	if (ret)
		return ret;

	ret = spinand_write_reg_op(spinand, REG_CFG, cfg_reg | BIT(6));
	if (ret)
		return ret;

	memset(&pos, 0, sizeof(pos));

	req = (struct nand_page_io_req){
		.pos = pos,
		.dataoffs = 0,
		.datalen = 256 * CASN_PAGE_V1_COPIES,
		.databuf.in = (u8 *)casn,
		.mode = MTD_OPS_AUTO_OOB,
	};

	for (i = 0; i < sizeof(casn_offset)/sizeof(uint8_t); i++) {
		req.pos.page = casn_offset[i];
		ret = spinand_load_page_op(spinand, &req);
		if (ret)
			goto finish;

		ret = spinand_wait(spinand, &status);
		if (ret < 0)
			goto finish;

		op = (struct spi_mem_op) SPINAND_PAGE_READ_FROM_CACHE_OP(
			false, 768, 1, (u8 *)casn, 256 * CASN_PAGE_V1_COPIES);
		ret = spi_mem_exec_op(spinand->slave, &op);
		if (ret < 0)
			goto finish;

		check_ret = spinand_check_casn(spinand, casn, sel);
		if (!check_ret)
			break;
	}

finish:
	/* We need to restore configuration register. */
	final_ret = spinand_write_reg_op(spinand, REG_CFG, cfg_reg);
	if (final_ret)
		return final_ret;

	if (check_ret) {
		dev_err(dev, "CASN page check failed\n");
		return check_ret;
	}

	if (ret)
		dev_err(dev, "CASN page read failed\n");

	return ret;
}

static int spinand_id_detect(struct spinand_device *spinand)
{
	u8 *id = spinand->id.data;
	int ret;

	ret = spinand_read_id_op(spinand, 0, 0, id);
	if (ret)
		return ret;
	ret = spinand_manufacturer_match(spinand, SPINAND_READID_METHOD_OPCODE);
	if (!ret)
		return 0;

	ret = spinand_read_id_op(spinand, 1, 0, id);
	if (ret)
		return ret;
	ret = spinand_manufacturer_match(spinand,
					 SPINAND_READID_METHOD_OPCODE_ADDR);
	if (!ret)
		return 0;

	ret = spinand_read_id_op(spinand, 0, 1, id);
	if (ret)
		return ret;
	ret = spinand_manufacturer_match(spinand,
					 SPINAND_READID_METHOD_OPCODE_DUMMY);

	return ret;
}

static int spinand_manufacturer_init(struct spinand_device *spinand)
{
	if (!spinand->use_casn && spinand->manufacturer->ops->init)
		return spinand->manufacturer->ops->init(spinand);

	return 0;
}

static void spinand_manufacturer_cleanup(struct spinand_device *spinand)
{
	/* Release manufacturer private data */
	if (!spinand->use_casn && spinand->manufacturer->ops->cleanup)
		return spinand->manufacturer->ops->cleanup(spinand);
}

static const struct spi_mem_op *
spinand_select_op_variant(struct spinand_device *spinand,
			  const struct spinand_op_variants *variants)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	for (i = 0; i < variants->nops; i++) {
		struct spi_mem_op op = variants->ops[i];
		unsigned int nbytes;
		int ret;

		nbytes = nanddev_per_page_oobsize(nand) +
			 nanddev_page_size(nand);

		while (nbytes) {
			op.data.nbytes = nbytes;
			ret = spi_mem_adjust_op_size(spinand->slave, &op);
			if (ret)
				break;

			if (!spi_mem_supports_op(spinand->slave, &op))
				break;

			nbytes -= op.data.nbytes;
		}

		if (!nbytes)
			return &variants->ops[i];
	}

	return NULL;
}

static int spinand_setup_slave(struct spinand_device *spinand,
			       const struct spinand_info *spinand_info)
{
	struct spi_slave *slave = spinand->slave;
	struct udevice *bus = slave->dev->parent;
	struct dm_spi_ops *ops = spi_get_ops(bus);

	if (!ops->setup_for_spinand)
		return 0;

	return ops->setup_for_spinand(slave, spinand_info);
}

/**
 * spinand_match_and_init() - Try to find a match between a device ID and an
 *			      entry in a spinand_info table
 * @spinand: SPI NAND object
 * @table: SPI NAND device description table
 * @table_size: size of the device description table
 * @rdid_method: read id method to match
 *
 * Match between a device ID retrieved through the READ_ID command and an
 * entry in the SPI NAND description table. If a match is found, the spinand
 * object will be initialized with information provided by the matching
 * spinand_info entry.
 *
 * Return: 0 on success, a negative error code otherwise.
 */
int spinand_match_and_init(struct spinand_device *spinand,
			   const struct spinand_info *table,
			   unsigned int table_size,
			   enum spinand_readid_method rdid_method)
{
	u8 *id = spinand->id.data;
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;
	int ret;

	for (i = 0; i < table_size; i++) {
		const struct spinand_info *info = &table[i];
		const struct spi_mem_op *op;

		if (rdid_method != info->devid.method)
			continue;

		if (memcmp(id + 1, info->devid.id, info->devid.len))
			continue;

		ret = spinand_setup_slave(spinand, info);
		if (ret)
			return ret;

		nand->memorg = table[i].memorg;
		nand->eccreq = table[i].eccreq;
		spinand->eccinfo = table[i].eccinfo;
		spinand->flags = table[i].flags;
		spinand->id.len = 1 + table[i].devid.len;
		spinand->select_target = table[i].select_target;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.read_cache);
		if (!op)
			return -ENOTSUPP;

		spinand->op_templates.read_cache = op;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.write_cache);
		if (!op)
			return -ENOTSUPP;

		spinand->op_templates.write_cache = op;

		op = spinand_select_op_variant(spinand,
					       info->op_variants.update_cache);
		spinand->op_templates.update_cache = op;

		return 0;
	}

	return -ENOTSUPP;
}

static int spinand_casn_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	int sectionp;
	struct CASN_OOB *co = spinand->casn_oob;

	sectionp = spinand->base.memorg.pagesize/mtd->ecc_step_size;
	if (section >= sectionp)
		return -ERANGE;

	if (co->layout_type == OOB_DISCRETE) {
		region->offset = co->ecc_parity_start +
				 (co->free_length + co->ecc_parity_space)
				 * section;
	} else if (co->layout_type == OOB_CONTINUOUS) {
		region->offset = co->ecc_parity_start + co->ecc_parity_space * section;
	}
	region->length = co->ecc_parity_real_length;

	return 0;
}

static int spinand_casn_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	struct spinand_device *spinand = mtd_to_spinand(mtd);
	int sectionp;
	struct CASN_OOB *co = spinand->casn_oob;

	sectionp = spinand->base.memorg.pagesize/mtd->ecc_step_size;
	if (section >= sectionp)
		return -ERANGE;

	if (!section) {
		region->offset = co->free_start + co->bbm_length;
		region->length = co->free_length - co->bbm_length;
	} else {
		if (co->layout_type == OOB_DISCRETE) {
			region->offset = co->free_start +
					 (co->free_length +
					  co->ecc_parity_space) * section;
		} else if (co->layout_type == OOB_CONTINUOUS) {
			region->offset = co->free_start +
					 co->free_length * section;
		}
		region->length = co->free_length;
	}

	return 0;
}

static const struct mtd_ooblayout_ops spinand_casn_ooblayout = {
	.ecc = spinand_casn_ooblayout_ecc,
	.rfree = spinand_casn_ooblayout_free,
};

static int spinand_set_read_op_variants(struct spinand_device *spinand,
					struct nand_casn *casn)
{
	struct spinand_op_variants casn_read_cache_variants;
	u16 sdr_read_cap = be16_to_cpu(casn->sdr_read_cap);
	struct spi_mem_op *read_ops;
	const struct spi_mem_op *op;
	int i = 0;

	read_ops = devm_kzalloc(spinand->slave->dev,
				sizeof(struct spi_mem_op) *
				hweight16(sdr_read_cap),
				GFP_KERNEL);
	if (!read_ops)
		return -ENOMEM;

	if (FIELD_GET(SDR_READ_1_4_4, sdr_read_cap)) {
		read_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PAGE_READ_FROM_CACHE_QUADIO_OP(
				casn->sdr_read_1_4_4.addr_nbytes, 0,
				casn->sdr_read_1_4_4.dummy_nbytes, NULL, 0
			);
		i++;
	}
	if (FIELD_GET(SDR_READ_1_1_4, sdr_read_cap)) {
		read_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PAGE_READ_FROM_CACHE_X4_OP(
				casn->sdr_read_1_1_4.addr_nbytes, 0,
				casn->sdr_read_1_1_4.dummy_nbytes, NULL, 0
			);
		i++;
	}
	if (FIELD_GET(SDR_READ_1_2_2, sdr_read_cap)) {
		read_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PAGE_READ_FROM_CACHE_DUALIO_OP(
				casn->sdr_read_1_2_2.addr_nbytes, 0,
				casn->sdr_read_1_2_2.dummy_nbytes, NULL, 0
			);
		i++;
	}
	if (FIELD_GET(SDR_READ_1_1_2, sdr_read_cap)) {
		read_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PAGE_READ_FROM_CACHE_X2_OP(
				casn->sdr_read_1_1_2.addr_nbytes, 0,
				casn->sdr_read_1_1_2.dummy_nbytes, NULL, 0
			);
		i++;
	}
	if (FIELD_GET(SDR_READ_1_1_1_FAST, sdr_read_cap)) {
		read_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PAGE_READ_FROM_CACHE_OP(
				true, casn->sdr_read_1_1_1_fast.addr_nbytes, 0,
				casn->sdr_read_1_1_1_fast.dummy_nbytes, NULL, 0
			);
		i++;
	}
	if (FIELD_GET(SDR_READ_1_1_1, sdr_read_cap)) {
		read_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PAGE_READ_FROM_CACHE_OP(
				false, casn->sdr_read_1_1_1.addr_nbytes, 0,
				casn->sdr_read_1_1_1.dummy_nbytes, NULL, 0
			);
		i++;
	}

	casn_read_cache_variants = (struct spinand_op_variants){
		.ops = read_ops,
		.nops = hweight16(sdr_read_cap),
	};

	op = spinand_select_op_variant(spinand, &casn_read_cache_variants);
	if (!op) {
		devm_kfree(spinand->slave->dev, read_ops);
		return -ENOTSUPP;
	}
	spinand->op_templates.read_cache = op;

	return 0;
}

static int spinand_set_write_op_variants(struct spinand_device *spinand,
					 struct nand_casn *casn)
{
	struct spinand_op_variants casn_write_cache_variants;
	struct spi_mem_op *write_ops;
	const struct spi_mem_op *op;
	int i = 0;

	write_ops = devm_kzalloc(spinand->slave->dev,
				 sizeof(struct spi_mem_op) *
				 hweight8(casn->sdr_write_cap),
				 GFP_KERNEL);
	if (!write_ops)
		return -ENOMEM;

	if (FIELD_GET(SDR_WRITE_1_1_4, casn->sdr_write_cap)) {
		write_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PROG_LOAD_X4(
				true, casn->sdr_write_1_1_4.addr_nbytes, 0,
				NULL, 0);
		i++;
	}
	if (FIELD_GET(SDR_WRITE_1_1_1, casn->sdr_write_cap)) {
		write_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PROG_LOAD(
				true, casn->sdr_write_1_1_1.addr_nbytes, 0,
				NULL, 0);
		i++;
	}

	casn_write_cache_variants = (struct spinand_op_variants){
		.ops = write_ops,
		.nops = hweight8(casn->sdr_write_cap),
	};

	op = spinand_select_op_variant(spinand, &casn_write_cache_variants);
	if (!op) {
		devm_kfree(spinand->slave->dev, write_ops);
		return -ENOTSUPP;
	}
	spinand->op_templates.write_cache = op;

	return 0;
}

static int spinand_set_update_op_variants(struct spinand_device *spinand,
					  struct nand_casn *casn)
{
	struct spinand_op_variants casn_update_cache_variants;
	struct spi_mem_op *update_ops;
	const struct spi_mem_op *op;
	int i = 0;

	update_ops = devm_kzalloc(spinand->slave->dev,
				  sizeof(struct spi_mem_op) *
				  hweight8(casn->sdr_update_cap),
				  GFP_KERNEL);
	if (!update_ops)
		return -ENOMEM;

	if (FIELD_GET(SDR_UPDATE_1_1_4, casn->sdr_update_cap)) {
		update_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PROG_LOAD_X4(
				false, casn->sdr_update_1_1_4.addr_nbytes, 0,
				NULL, 0);
		i++;
	}
	if (FIELD_GET(SDR_UPDATE_1_1_1, casn->sdr_update_cap)) {
		update_ops[i] = (struct spi_mem_op)
			SPINAND_CASN_PROG_LOAD(
				false, casn->sdr_update_1_1_1.addr_nbytes, 0,
				NULL, 0);
		i++;
	}

	casn_update_cache_variants = (struct spinand_op_variants){
		.ops = update_ops,
		.nops = hweight8(casn->sdr_update_cap),
	};

	op = spinand_select_op_variant(spinand, &casn_update_cache_variants);
	if (!op) {
		devm_kfree(spinand->slave->dev, update_ops);
		return -ENOTSUPP;
	}
	spinand->op_templates.update_cache = op;

	return 0;
}

static int spinand_init_via_casn(struct spinand_device *spinand,
				 struct nand_casn *casn)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	u32 val;
	int ret;
	int i;

	/* Set members of nand->memorg via CASN. */
	for (i = 0; i < 9; i++) {
		val = be32_to_cpu(*(&casn->bits_per_cell + i));
		memcpy((u32 *)&nand->memorg.bits_per_cell + i, &val, sizeof(u32));
	}
	nand->eccreq.strength = be32_to_cpu(casn->ecc_strength);
	nand->eccreq.step_size = be32_to_cpu(casn->ecc_step_size);
	spinand->flags = casn->flags;

	if (spinand->flags & SPINAND_SUP_ADV_ECC_STATUS) {
		spinand->eccinfo = (struct spinand_ecc_info) {
			&spinand_casn_get_ecc_status, &spinand_casn_ooblayout};
	} else {
		spinand->eccinfo = (struct spinand_ecc_info) {
			NULL, &spinand_casn_ooblayout };
	}

	spinand->advecc_high_ops = devm_kzalloc(spinand->slave->dev,
						sizeof(struct spi_mem_op),
						GFP_KERNEL);
	if (!spinand->advecc_high_ops)
		return -ENOMEM;
	spinand->advecc_low_ops = devm_kzalloc(spinand->slave->dev,
					       sizeof(struct spi_mem_op),
					       GFP_KERNEL);
	if (!spinand->advecc_low_ops)
		return -ENOMEM;
	spinand->casn_oob = devm_kzalloc(spinand->slave->dev,
					 sizeof(struct CASN_OOB),
					 GFP_KERNEL);
	if (!spinand->casn_oob)
		return -ENOMEM;
	spinand->advecc_high = devm_kzalloc(spinand->slave->dev,
					    sizeof(struct CASN_ADVECC),
					    GFP_KERNEL);
	if (!spinand->advecc_high)
		return -ENOMEM;
	spinand->advecc_low = devm_kzalloc(spinand->slave->dev,
					   sizeof(struct CASN_ADVECC),
					   GFP_KERNEL);
	if (!spinand->advecc_low)
		return -ENOMEM;

	*spinand->advecc_high_ops = (struct spi_mem_op)
		SPINAND_CASN_ADVECC_OP(casn->ecc_status_high, spinand->scratchbuf);
	*spinand->advecc_low_ops = (struct spi_mem_op)
		SPINAND_CASN_ADVECC_OP(casn->ecc_status_low, spinand->scratchbuf);

	memcpy(spinand->casn_oob, &casn->casn_oob, sizeof(struct CASN_OOB));

	spinand->advecc_high->cmd = casn->ecc_status_high.cmd;
	spinand->advecc_high->mask = be16_to_cpu(casn->ecc_status_high.status_mask);
	spinand->advecc_high->shift = spinand->advecc_high->mask ?
				      ffs(spinand->advecc_high->mask)-1 : 0;
	spinand->advecc_high->pre_op = casn->ecc_status_high.pre_op;
	spinand->advecc_high->pre_mask = casn->ecc_status_high.pre_mask;

	spinand->advecc_low->cmd = casn->ecc_status_low.cmd;
	spinand->advecc_low->mask = be16_to_cpu(casn->ecc_status_low.status_mask);
	spinand->advecc_low->shift = spinand->advecc_low->mask ?
				     ffs(spinand->advecc_low->mask)-1 : 0;
	spinand->advecc_low->pre_op = casn->ecc_status_low.pre_op;
	spinand->advecc_low->pre_mask = casn->ecc_status_low.pre_mask;

	spinand->advecc_low_bitcnt = hweight16(spinand->advecc_low->mask);

	spinand->advecc_noerr_status = casn->advecc_noerr_status;
	spinand->advecc_uncor_status = casn->advecc_uncor_status;
	spinand->advecc_post_op = casn->advecc_post_op;
	spinand->advecc_post_mask = casn->advecc_post_mask;
	spinand->eccsr_math_op[0] = eccsr_none_op;
	spinand->eccsr_math_op[1] = eccsr_and_op;
	spinand->eccsr_math_op[2] = eccsr_add_op;
	spinand->eccsr_math_op[3] = eccsr_minus_op;
	spinand->eccsr_math_op[4] = eccsr_mul_op;

	ret = spinand_set_read_op_variants(spinand, casn);
	if (ret < 0)
		return ret;
	ret = spinand_set_write_op_variants(spinand, casn);
	if (ret < 0)
		return ret;
	ret = spinand_set_update_op_variants(spinand, casn);
	if (ret < 0)
		return ret;

	return 0;
}

static void spinand_dump_casn(struct spinand_device *spinand, struct nand_casn *casn)
{
	int i;

	dev_dbg(spinand->slave->dev,
		"---Start dumping full CASN page---\n");
	for (i = 0; i < 64; i++)
		pr_debug("0x%08x", *((u32 *)casn + i));

	pr_debug("** Dump critical fields **\n");
	pr_debug("signature: 0x%04x\n", be32_to_cpu(casn->signature));
	pr_debug("version: v%u.%u\n", casn->version >> 4, casn->version & 0xf);
	pr_debug("[Memory Organization]\n");
	pr_debug("  bits_per_cell: %d\n", be32_to_cpu(casn->bits_per_cell));
	pr_debug("  bytes_per_page: %d\n", be32_to_cpu(casn->bytes_per_page));
	pr_debug("  spare_bytes_per_page: %d\n",
		 be32_to_cpu(casn->spare_bytes_per_page));
	pr_debug("  pages_per_block: %d\n",
		 be32_to_cpu(casn->pages_per_block));
	pr_debug("  blocks_per_lun: %d\n", be32_to_cpu(casn->blocks_per_lun));
	pr_debug("  max_bb_per_lun: %d\n", be32_to_cpu(casn->max_bb_per_lun));
	pr_debug("  planes_per_lun: %d\n", be32_to_cpu(casn->planes_per_lun));
	pr_debug("  luns_per_target: %d\n",
		 be32_to_cpu(casn->luns_per_target));
	pr_debug("  total_target: %d\n", be32_to_cpu(casn->total_target));
	pr_debug("[flags]\n");
	pr_debug("  0. Have QE bit? %s\n",
		casn->flags & SPINAND_HAS_QE_BIT ? "Yes" : "No");
	pr_debug("  1. Have continuous read feature bit? %s\n",
		casn->flags & SPINAND_HAS_CR_FEAT_BIT ? "Yes" : "No");
	pr_debug("  2. Support continuous read? %s\n",
		casn->flags & SPINAND_SUP_CR ? "Yes" : "No");
	pr_debug("  3. Support on-die ECC? %s\n",
		casn->flags & SPINAND_SUP_ON_DIE_ECC ? "Yes" : "No");
	pr_debug("  4. Support legacy ECC status? %s\n",
		casn->flags & SPINAND_SUP_LEGACY_ECC_STATUS ? "Yes" : "No");
	pr_debug("  5. Support advanced ECC status? %s\n",
		casn->flags & SPINAND_SUP_ADV_ECC_STATUS ? "Yes" : "No");
	pr_debug("  6. ECC parity readable? %s\n",
		casn->flags & SPINAND_ECC_PARITY_READABLE ? "Yes" : "No");
	pr_debug("[R/W ability]\n");
	pr_debug("  read ability: %x\n", be16_to_cpu(casn->sdr_read_cap));
	pr_debug("  write ability: %x\n", casn->sdr_write_cap);
	pr_debug("  update ability: %x\n", casn->sdr_update_cap);
	pr_debug("advanced ECC no error state: %x\n",
		 casn->advecc_noerr_status);
	pr_debug("advecced ECC uncorrectable state: %x\n",
		 casn->advecc_uncor_status);
	pr_debug("CRC: 0x%04x\n", be16_to_cpu(casn->crc));

	dev_dbg(spinand->slave->dev,
		"---Dumping full CASN page ends here.---\n");
}

static int spinand_detect(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	struct udevice *dev = spinand->slave->dev;
	struct nand_casn *casn;
	char manufacturer[14];
	unsigned int sel = 0;
	char model[17];
	int ret;

	ret = spinand_reset_op(spinand);
	if (ret)
		return ret;

	spinand->use_casn = false;
	casn = kzalloc((sizeof(struct nand_casn) * CASN_PAGE_V1_COPIES), GFP_KERNEL);
	if (!casn)
		return -ENOMEM;

	ret = spinand_casn_detect(spinand, casn, &sel);
	if (!ret) {
		spinand->use_casn = true;
		strncpy(manufacturer, casn[sel].manufacturer, sizeof(manufacturer)-1);
		sanitize_string(manufacturer, sizeof(manufacturer));
		strncpy(model, casn[sel].model, sizeof(model)-1);
		sanitize_string(model, sizeof(model));

		spinand_dump_casn(spinand, casn + sel);

		ret = spinand_init_via_casn(spinand, casn + sel);
		if (ret)
			dev_err(dev, "Initilize spinand via CASN failed: %d\n", ret);
	}

	if (ret < 0) {
		dev_warn(dev, "Fallback to read ID\n");

		ret = spinand_reset_op(spinand);
		if (ret)
			goto free_casn;
		ret = spinand_id_detect(spinand);
		if (ret) {
			dev_err(dev, "unknown raw ID %*phN\n", SPINAND_MAX_ID_LEN,
				spinand->id.data);
			goto free_casn;
		}
	}

	if (nand->memorg.ntargets > 1 && !spinand->select_target) {
		dev_err(dev,
			"SPI NANDs with more than one die must implement ->select_target()\n");
		return -EINVAL;
		goto free_casn;
	}

	if (spinand->use_casn) {
		dev_info(spinand->slave->dev,
			 "%s %s SPI NAND was found.\n", manufacturer, model);
	} else {
		dev_info(spinand->slave->dev,
			 "%s SPI NAND was found.\n", spinand->manufacturer->name);
	}

	dev_info(dev, "%llu MiB, block size: %zu KiB, page size: %zu, OOB size: %u\n",
		 nanddev_size(nand) >> 20, nanddev_eraseblock_size(nand) >> 10,
		 nanddev_page_size(nand), nanddev_per_page_oobsize(nand));

free_casn:
	kfree(casn);

	return ret;
}

static int spinand_noecc_ooblayout_ecc(struct mtd_info *mtd, int section,
				       struct mtd_oob_region *region)
{
	return -ERANGE;
}

static int spinand_noecc_ooblayout_free(struct mtd_info *mtd, int section,
					struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	/* Reserve 2 bytes for the BBM. */
	region->offset = 2;
	region->length = 62;

	return 0;
}

static const struct mtd_ooblayout_ops spinand_noecc_ooblayout = {
	.ecc = spinand_noecc_ooblayout_ecc,
	.rfree = spinand_noecc_ooblayout_free,
};

static int spinand_init(struct spinand_device *spinand)
{
	struct mtd_info *mtd = spinand_to_mtd(spinand);
	struct nand_device *nand = mtd_to_nanddev(mtd);
	int ret, i;

	/*
	 * We need a scratch buffer because the spi_mem interface requires that
	 * buf passed in spi_mem_op->data.buf be DMA-able.
	 */
	spinand->scratchbuf = kzalloc(SPINAND_MAX_ID_LEN, GFP_KERNEL);
	if (!spinand->scratchbuf)
		return -ENOMEM;

	ret = spinand_detect(spinand);
	if (ret)
		goto err_free_bufs;

	/*
	 * Use kzalloc() instead of devm_kzalloc() here, because some drivers
	 * may use this buffer for DMA access.
	 * Memory allocated by devm_ does not guarantee DMA-safe alignment.
	 */
	spinand->databuf = kzalloc(nanddev_page_size(nand) +
			       nanddev_per_page_oobsize(nand),
			       GFP_KERNEL);
	if (!spinand->databuf) {
		ret = -ENOMEM;
		goto err_free_bufs;
	}

	spinand->oobbuf = spinand->databuf + nanddev_page_size(nand);

	ret = spinand_init_cfg_cache(spinand);
	if (ret)
		goto err_free_bufs;

	ret = spinand_init_quad_enable(spinand);
	if (ret)
		goto err_free_bufs;

	ret = spinand_upd_cfg(spinand, CFG_OTP_ENABLE, 0);
	if (ret)
		goto err_free_bufs;

	ret = spinand_manufacturer_init(spinand);
	if (ret) {
		dev_err(spinand->slave->dev,
			"Failed to initialize the SPI NAND chip (err = %d)\n",
			ret);
		goto err_free_bufs;
	}

	/* After power up, all blocks are locked, so unlock them here. */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		ret = spinand_select_target(spinand, i);
		if (ret)
			goto err_manuf_cleanup;

		ret = spinand_lock_block(spinand, BL_ALL_UNLOCKED);
		if (ret)
			goto err_manuf_cleanup;
	}

	ret = nanddev_init(nand, &spinand_ops, THIS_MODULE);
	if (ret)
		goto err_manuf_cleanup;

	/*
	 * Right now, we don't support ECC, so let the whole oob
	 * area is available for user.
	 */
	mtd->_read_oob = spinand_mtd_read;
	mtd->_write_oob = spinand_mtd_write;
	mtd->_block_isbad = spinand_mtd_block_isbad;
	mtd->_block_markbad = spinand_mtd_block_markbad;
	mtd->_block_isreserved = spinand_mtd_block_isreserved;
	mtd->_erase = spinand_mtd_erase;

	if (spinand->eccinfo.ooblayout)
		mtd_set_ooblayout(mtd, spinand->eccinfo.ooblayout);
	else
		mtd_set_ooblayout(mtd, &spinand_noecc_ooblayout);

	ret = mtd_ooblayout_count_freebytes(mtd);
	if (ret < 0)
		goto err_cleanup_nanddev;

	mtd->oobavail = ret;

	return 0;

err_cleanup_nanddev:
	nanddev_cleanup(nand);

err_manuf_cleanup:
	spinand_manufacturer_cleanup(spinand);

err_free_bufs:
	kfree(spinand->databuf);
	kfree(spinand->scratchbuf);
	return ret;
}

static void spinand_cleanup(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	nanddev_cleanup(nand);
	spinand_manufacturer_cleanup(spinand);
	kfree(spinand->databuf);
	kfree(spinand->scratchbuf);
}

static int spinand_bind(struct udevice *dev)
{
	if (blk_enabled()) {
		struct spinand_plat *plat = dev_get_plat(dev);
		int ret;

		if (CONFIG_IS_ENABLED(MTD_BLOCK)) {
			ret = mtd_bind(dev, &plat->mtd);
			if (ret)
				return ret;
		}

		if (CONFIG_IS_ENABLED(UBI_BLOCK))
			return ubi_bind(dev);
	}

	return 0;
}

static int spinand_probe(struct udevice *dev)
{
	struct spinand_device *spinand = dev_get_priv(dev);
	struct spi_slave *slave = dev_get_parent_priv(dev);
	struct mtd_info *mtd = dev_get_uclass_priv(dev);
	struct nand_device *nand = spinand_to_nand(spinand);
	struct spinand_plat *plat = dev_get_plat(dev);
	int ret;

#ifndef __UBOOT__
	spinand = devm_kzalloc(&mem->spi->dev, sizeof(*spinand),
			       GFP_KERNEL);
	if (!spinand)
		return -ENOMEM;

	spinand->spimem = mem;
	spi_mem_set_drvdata(mem, spinand);
	spinand_set_of_node(spinand, mem->spi->dev.of_node);
	mutex_init(&spinand->lock);

	mtd = spinand_to_mtd(spinand);
	mtd->dev.parent = &mem->spi->dev;
#else
	nand->mtd = mtd;
	mtd->priv = nand;
	mtd->dev = dev;
	mtd->name = malloc(20);
	if (!mtd->name)
		return -ENOMEM;
	sprintf(mtd->name, "spi-nand%d", spi_nand_idx++);
	spinand->slave = slave;
	spinand_set_ofnode(spinand, dev_ofnode(dev));
#endif

	ret = spinand_init(spinand);
	if (ret)
		return ret;

#ifndef __UBOOT__
	ret = mtd_device_register(mtd, NULL, 0);
#else
	ret = add_mtd_device(mtd);
#endif
	if (ret)
		goto err_spinand_cleanup;

	plat->mtd = mtd;

	return 0;

err_spinand_cleanup:
	spinand_cleanup(spinand);

	return ret;
}

#ifndef __UBOOT__
static int spinand_remove(struct udevice *slave)
{
	struct spinand_device *spinand;
	struct mtd_info *mtd;
	int ret;

	spinand = spi_mem_get_drvdata(slave);
	mtd = spinand_to_mtd(spinand);
	free(mtd->name);

	ret = mtd_device_unregister(mtd);
	if (ret)
		return ret;

	spinand_cleanup(spinand);

	return 0;
}

static const struct spi_device_id spinand_ids[] = {
	{ .name = "spi-nand" },
	{ /* sentinel */ },
};

#ifdef CONFIG_OF
static const struct of_device_id spinand_of_ids[] = {
	{ .compatible = "spi-nand" },
	{ /* sentinel */ },
};
#endif

static struct spi_mem_driver spinand_drv = {
	.spidrv = {
		.id_table = spinand_ids,
		.driver = {
			.name = "spi-nand",
			.of_match_table = of_match_ptr(spinand_of_ids),
		},
	},
	.probe = spinand_probe,
	.remove = spinand_remove,
};
module_spi_mem_driver(spinand_drv);

MODULE_DESCRIPTION("SPI NAND framework");
MODULE_AUTHOR("Peter Pan<peterpandong@micron.com>");
MODULE_LICENSE("GPL v2");
#endif /* __UBOOT__ */

static const struct udevice_id spinand_ids[] = {
	{ .compatible = "spi-nand" },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(spinand) = {
	.name = "spi_nand",
	.id = UCLASS_MTD,
	.of_match = spinand_ids,
	.priv_auto	= sizeof(struct spinand_device),
	.probe = spinand_probe,
	.bind = spinand_bind,
	.plat_auto = sizeof(struct spinand_plat),
};
