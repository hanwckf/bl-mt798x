// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common.h>
#include <dm.h>
#include <dm/uclass.h>
#include <malloc.h>
#include <mapmem.h>
#include <mtd.h>
#include <watchdog.h>

#include <nmbm/nmbm.h>

#include "mtk-snand.h"

static struct mtk_snand *snf;
static struct mtk_snand_chip_info cinfo;
static u32 oobavail;

#ifdef CONFIG_ENABLE_NAND_NMBM
static struct nmbm_instance *ni;

static int nmbm_lower_read_page(void *arg, uint64_t addr, void *buf, void *oob,
				enum nmbm_oob_mode mode)
{
	int ret;
	bool raw = mode == NMBM_MODE_RAW ? true : false;

	if (mode == NMBM_MODE_AUTO_OOB) {
		ret = mtk_snand_read_page_auto_oob(snf, addr, buf, oob,
			oobavail, NULL, false);
	} else {
		ret = mtk_snand_read_page(snf, addr, buf, oob, raw);
	}

	if (ret == -EBADMSG)
		return 1;
	else if (ret >= 0)
		return 0;

	return ret;
}

static int nmbm_lower_write_page(void *arg, uint64_t addr, const void *buf,
				 const void *oob, enum nmbm_oob_mode mode)
{
	bool raw = mode == NMBM_MODE_RAW ? true : false;

	if (mode == NMBM_MODE_AUTO_OOB) {
		return mtk_snand_write_page_auto_oob(snf, addr, buf, oob,
			oobavail, NULL, false);
	}

	return mtk_snand_write_page(snf, addr, buf, oob, raw);
}

static int nmbm_lower_erase_block(void *arg, uint64_t addr)
{
	return mtk_snand_erase_block(snf, addr);
}

static int nmbm_lower_is_bad_block(void *arg, uint64_t addr)
{
	return mtk_snand_block_isbad(snf, addr);
}

static int nmbm_lower_mark_bad_block(void *arg, uint64_t addr)
{
	return mtk_snand_block_markbad(snf, addr);
}

static void nmbm_lower_log(void *arg, enum nmbm_log_category level,
			   const char *fmt, va_list ap)
{
	vprintf(fmt, ap);
}

static int nmbm_init(void)
{
	struct nmbm_lower_device nld;
	size_t ni_size;
	int ret;

	memset(&nld, 0, sizeof(nld));

	nld.flags = NMBM_F_CREATE;
	nld.max_ratio = CONFIG_NMBM_MAX_RATIO;
	nld.max_reserved_blocks = CONFIG_NMBM_MAX_BLOCKS;

	nld.size = cinfo.chipsize;
	nld.erasesize = cinfo.blocksize;
	nld.writesize = cinfo.pagesize;
	nld.oobsize = cinfo.sparesize;
	nld.oobavail = oobavail;

	nld.read_page = nmbm_lower_read_page;
	nld.write_page = nmbm_lower_write_page;
	nld.erase_block = nmbm_lower_erase_block;
	nld.is_bad_block = nmbm_lower_is_bad_block;
	nld.mark_bad_block = nmbm_lower_mark_bad_block;

	nld.logprint = nmbm_lower_log;

	ni_size = nmbm_calc_structure_size(&nld);
	ni = malloc(ni_size);
	if (!ni) {
		printf("Failed to allocate memory (0x%u) for NMBM instance\n",
		       ni_size);
		return -ENOMEM;
	}

	memset(ni, 0, ni_size);

	printf("Initializing NMBM ...\n");

	ret = nmbm_attach(&nld, ni);
	if (ret) {
		ni = NULL;
		return ret;
	}

	return 0;
}

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dst)
{
	size_t retlen;

	if (!ni)
		return -ENODEV;

	nmbm_read_range(ni, offs, size, dst, NMBM_MODE_PLACE_OOB, &retlen);
	if (retlen != size)
		return -EIO;

	return 0;
}

#else
static u8 *page_cache;

int nand_spl_load_image(uint32_t offs, unsigned int size, void *dst)
{
	u32 sizeremain = size, chunksize, leading;
	uint32_t off = offs, writesize_mask = cinfo.pagesize - 1;
	uint8_t *ptr = dst;
	int ret;

	if (!snf)
		return -ENODEV;

	while (sizeremain) {
		WATCHDOG_RESET();

		leading = off & writesize_mask;
		chunksize = cinfo.pagesize - leading;
		if (chunksize > sizeremain)
			chunksize = sizeremain;

		if (chunksize == cinfo.pagesize) {
			ret = mtk_snand_read_page(snf, off - leading, ptr,
						  NULL, false);
			if (ret)
				break;
		} else {
			ret = mtk_snand_read_page(snf, off - leading,
						  page_cache, NULL, false);
			if (ret)
				break;

			memcpy(ptr, page_cache + leading, chunksize);
		}

		off += chunksize;
		ptr += chunksize;
		sizeremain -= chunksize;
	}

	return ret;
}
#endif

void nand_init(void)
{
	struct mtk_snand_platdata mtk_snand_pdata = {};
	struct udevice *dev;
	fdt_addr_t base;
	int ret;

	ret = uclass_get_device_by_driver(UCLASS_MTD, DM_DRIVER_GET(mtk_snand),
					  &dev);
	if (ret) {
		printf("mtk-snand-spl: Device instance not found!\n");
		return;
	}

	base = dev_read_addr_name(dev, "nfi");
	if (base == FDT_ADDR_T_NONE) {
		printf("mtk-snand-spl: NFI base not set\n");
		return;
	}
	mtk_snand_pdata.nfi_base = map_sysmem(base, 0);

	base = dev_read_addr_name(dev, "ecc");
	if (base == FDT_ADDR_T_NONE) {
		printf("mtk-snand-spl: ECC base not set\n");
		return;
	}
	mtk_snand_pdata.ecc_base = map_sysmem(base, 0);

	mtk_snand_pdata.soc = dev_get_driver_data(dev);
	mtk_snand_pdata.quad_spi = dev_read_bool(dev, "quad-spi");

	ret = mtk_snand_init(NULL, &mtk_snand_pdata, &snf);
	if (ret) {
		printf("mtk-snand-spl: failed to initialize SPI-NAND\n");
		return;
	}

	mtk_snand_get_chip_info(snf, &cinfo);

	oobavail = cinfo.num_sectors * (cinfo.fdm_size - 1);

	printf("SPI-NAND: %s (%uMB)\n", cinfo.model,
	       (u32)(cinfo.chipsize >> 20));

#ifdef CONFIG_ENABLE_NAND_NMBM
	nmbm_init();
#else
	page_cache = malloc(cinfo.pagesize + cinfo.sparesize);
	if (!page_cache) {
		mtk_snand_cleanup(snf);
		printf("mtk-snand-spl: failed to allocate page cache\n");
	}
#endif
}

void nand_deselect(void)
{

}

static const struct udevice_id mtk_snand_ids[] = {
	{ .compatible = "mediatek,mt7622-snand", .data = SNAND_SOC_MT7622 },
	{ .compatible = "mediatek,mt7629-snand", .data = SNAND_SOC_MT7629 },
	{ .compatible = "mediatek,mt7981-snand", .data = SNAND_SOC_MT7981 },
	{ .compatible = "mediatek,mt7986-snand", .data = SNAND_SOC_MT7986 },
	{ /* sentinel */ },
};

U_BOOT_DRIVER(mtk_snand) = {
	.name = "mtk-snand",
	.id = UCLASS_MTD,
	.of_match = mtk_snand_ids,
	.flags = DM_FLAG_PRE_RELOC,
};
