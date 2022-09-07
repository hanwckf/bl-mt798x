/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/delay_timer.h>
#include <drivers/io/io_driver.h>
#include <lib/utils_def.h>
#include <bl2_boot_dev.h>
#include <mt7981_def.h>
#include <mt7981_gpio.h>
#include <common/debug.h>
#include <plat/common/platform.h>
#include <lib/mmio.h>
#include <string.h>

#include <mtk-snand.h>
#include <mempool.h>

#ifdef NMBM
#include <nmbm/nmbm.h>
#endif
#define FIP_BASE			0x380000
#define FIP_SIZE			0x200000

#ifndef NMBM_MAX_RATIO
#define NMBM_MAX_RATIO			1
#endif

#ifndef NMBM_MAX_RESERVED_BLOCKS
#define NMBM_MAX_RESERVED_BLOCKS	256
#endif
static struct mtk_snand *snf;
static struct mtk_snand_chip_info cinfo;
static uint32_t oobavail;
#ifdef NMBM
static struct nmbm_instance *ni;

static int nmbm_lower_read_page(void *arg, uint64_t addr, void *buf, void *oob,
				enum nmbm_oob_mode mode)
{
	bool raw = mode == NMBM_MODE_RAW ? true : false;

	if (mode == NMBM_MODE_AUTO_OOB) {
		return mtk_snand_read_page_auto_oob(snf, addr, buf, oob,
			oobavail, NULL, false);
	}

	return mtk_snand_read_page(snf, addr, buf, oob, raw);
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
	int log_level;
	const char *prefix_str;

	switch (level) {
	case NMBM_LOG_DEBUG:
		log_level = LOG_LEVEL_VERBOSE;
		break;
	case NMBM_LOG_WARN:
		log_level = LOG_LEVEL_WARNING;
		break;
	case NMBM_LOG_ERR:
	case NMBM_LOG_EMERG:
		log_level = LOG_LEVEL_ERROR;
		break;
	default:
		log_level = LOG_LEVEL_NOTICE;
	}

	if (log_level > LOG_LEVEL)
		return;

	prefix_str = plat_log_get_prefix(log_level);

	while (*prefix_str != '\0') {
		(void)putchar(*prefix_str);
		prefix_str++;
	}

	vprintf(fmt, ap);
}

static int nmbm_init(void)
{
	struct nmbm_lower_device nld;
	size_t ni_size;
	int ret;

	memset(&nld, 0, sizeof(nld));

	nld.flags = NMBM_F_CREATE | NMBM_F_EMPTY_PAGE_ECC_OK;
	nld.max_ratio = NMBM_MAX_RATIO;
	nld.max_reserved_blocks = NMBM_MAX_RESERVED_BLOCKS;

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
	ni = mtk_mem_pool_alloc(ni_size);
	memset(ni, 0, ni_size);

	NOTICE("Initializing NMBM ...\n");

	ret = nmbm_attach(&nld, ni);
	if (ret) {
		ni = NULL;
		return ret;
	}

	return 0;
}

static size_t snand_read_range(int lba, uintptr_t buf, size_t size)
{
	size_t retlen = 0;

	nmbm_read_range(ni, lba * cinfo.pagesize, size, (void *)buf,
			NMBM_MODE_PLACE_OOB, &retlen);

	return retlen;
}
#else
static uint8_t *page_cache;

static size_t snand_read_range(int lba, uintptr_t buf, size_t size)
{
	size_t sizeremain = size, chunksize;
	uint64_t off = lba * cinfo.pagesize;
	uint8_t *ptr = (uint8_t *)buf;
	int ret = 0;

	if (!snf)
		return 0;

	while (sizeremain) {
		chunksize = cinfo.pagesize;
		if (chunksize > sizeremain)
			chunksize = sizeremain;

		ret = mtk_snand_read_page(snf, off, page_cache, NULL, false);
		if (ret < 0)
			break;

		memcpy(ptr, page_cache, chunksize);

		off += chunksize;
		ptr += chunksize;
		sizeremain -= chunksize;
	}

	return size - sizeremain;
}
#endif

static size_t snand_write_range(int lba, uintptr_t buf, size_t size)
{
	/* Do not use write in BL2 */
	return 0;
}

static io_block_dev_spec_t snand_dev_spec = {
	/* Buffer should not exceed BL33_BASE */
	.buffer = {
		.offset = 0x41000000,
		.length = 0xe00000,
	},

	.ops = {
		.read = snand_read_range,
		.write = snand_write_range,
	},
};

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset	= FIP_BASE,
	.length = FIP_SIZE,
};

static const struct mtk_snand_platdata mt7981_snand_pdata = {
	.nfi_base = (void *)NFI_BASE,
	.ecc_base = (void *)NFI_ECC_BASE,
	.soc = SNAND_SOC_MT7981,
	.quad_spi = true
};

static void snand_gpio_clk_setup(void)
{
	/* Reset */
	mmio_setbits_32(0x10001080, 1 << 2);
	udelay(1000);
	mmio_setbits_32(0x10001084, 1 << 2);

	/* TOPCKGEN CFG0 nfi1x */
	mmio_write_32(CLK_CFG_0_CLR, CLK_NFI1X_SEL_MASK);
	mmio_write_32(CLK_CFG_0_SET, CLK_NFI1X_52MHz << CLK_NFI1X_SEL_S);

	/* TOPCKGEN CFG0 spinfi */
	mmio_write_32(CLK_CFG_0_CLR, CLK_SPINFI_BCLK_SEL_MASK);
	mmio_write_32(CLK_CFG_0_SET, CLK_NFI1X_52MHz << CLK_SPINFI_BCLK_SEL_S);

	mmio_write_32(CLK_CFG_UPDATE, NFI1X_CK_UPDATE | SPINFI_CK_UPDATE);

	/* GPIO mode */
	mmio_clrsetbits_32(GPIO_MODE2,
		0x7 << GPIO_PIN21_S | 0x7 << GPIO_PIN20_S | 0x7 << GPIO_PIN19_S |
		0x7 << GPIO_PIN18_S | 0x7 << GPIO_PIN17_S | 0x7 << GPIO_PIN16_S,
		0x3 << GPIO_PIN21_S | 0x3 << GPIO_PIN20_S | 0x3 << GPIO_PIN19_S |
		0x3 << GPIO_PIN18_S | 0x3 << GPIO_PIN17_S | 0x3 << GPIO_PIN16_S);

	/* GPIO PUPD */
	mmio_clrsetbits_32(GPIO_RM_PUPD_CFG0, 0b111111 << SPI0_PUPD_S, 0b011001 << SPI0_PUPD_S);
	mmio_clrsetbits_32(GPIO_RM_R0_CFG0, 0b111111 << SPI0_R0_S, 0b100110 << SPI0_R0_S);
	mmio_clrsetbits_32(GPIO_RM_R1_CFG0, 0b111111 << SPI0_R1_S, 0b011001 << SPI0_R1_S);

	/* GPIO driving */
	mmio_clrsetbits_32(GPIO_RM_DRV_CFG0,
		0x7 << SPI0_WP_DRV_S   | 0x7 << SPI0_MOSI_DRV_S | 0x7 << SPI0_MISO_DRV_S |
		0x7 << SPI0_HOLD_DRV_S | 0x7 << SPI0_CS_DRV_S   | 0x7 << SPI0_CLK_DRV_S,
		0x2 << SPI0_WP_DRV_S   | 0x2 << SPI0_MOSI_DRV_S | 0x2 << SPI0_MISO_DRV_S |
		0x2 << SPI0_HOLD_DRV_S | 0x2 << SPI0_CS_DRV_S   | 0x3 << SPI0_CLK_DRV_S);
}

static int mt7981_snand_init(void)
{
	int ret;

	snand_gpio_clk_setup();

	ret = mtk_snand_init(NULL, &mt7981_snand_pdata, &snf);
	if (ret) {
		printf("failed\n");
		snf = NULL;
		return ret;
	}

	mtk_snand_get_chip_info(snf, &cinfo);
	oobavail = cinfo.num_sectors * (cinfo.fdm_size - 1);
	snand_dev_spec.block_size = cinfo.pagesize;
#ifndef NMBM
	page_cache = mtk_mem_pool_alloc(cinfo.pagesize + cinfo.sparesize);
#endif

	NOTICE("SPI-NAND: %s (%luMB)\n", cinfo.model, cinfo.chipsize >> 20);

	return ret;
}

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	int result;

	result = mt7981_snand_init();
	assert(result == 0);
#ifdef NMBM
	result = nmbm_init();
	assert(result == 0);
#endif

	result = register_io_dev_block(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)&snand_dev_spec,
			     boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
