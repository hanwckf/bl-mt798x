/*
 * Copyright (c) 2019-2022,  STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <mtk_spi_nand.h>
#include <lib/utils.h>

#include <platform_def.h>

#define SPI_NAND_MAX_ID_LEN		4U
#define DELAY_US_400MS			400000U
#define ETRON_ID			0xD5U
#define GIGADEVICE_ID			0xC8U
#define MACRONIX_ID			0xC2U
#define MICRON_ID			0x2CU
#define TOSHIBA_ID			0x98U
#define FORESEE_ID			0xCDU
#define ONFI_SIGNATURE			0x4F4E4649U
#define NAND_SIGNATURE			0x4E414E44U /* Kioxia/Toshiba use this*/
#define PARAMETER_PAGE_COPIES		3U

#define SPI_NAND_MEMORG(_ps, _ss, _ppb, _bpd, _ppd, _nd) \
	{ .pagesize = (_ps), .sparesize = (_ss), .pages_per_block = (_ppb), \
	  .blocks_per_die = (_bpd), .planes_per_die = (_ppd), .ndies = (_nd) }

#define SPI_NAND_MEMORG_512M_2K_64		SPI_NAND_MEMORG(2048, 64, 64, 512, 1, 1)
#define SPI_NAND_MEMORG_1G_2K_64		SPI_NAND_MEMORG(2048, 64, 64, 1024, 1, 1)
#define SPI_NAND_MEMORG_2G_2K_64		SPI_NAND_MEMORG(2048, 64, 64, 2048, 1, 1)
#define SPI_NAND_MEMORG_2G_2K_128		SPI_NAND_MEMORG(2048, 128, 64, 2048, 1, 1)
#define SPI_NAND_MEMORG_4G_4K_256		SPI_NAND_MEMORG(4096, 256, 64, 2048, 1, 1)

static struct spinand_device spinand_dev;

#pragma weak plat_get_spi_nand_data
int plat_get_spi_nand_data(struct spinand_device *device) {
	return 0;
}

#define SPI_NAND_ID(_id_dummy, _id_len, ...) {.id_dummy = (_id_dummy),	\
	.id_len = (_id_len), .id_data = { __VA_ARGS__ }}

#define SPI_NAND_INFO(_model, _nand_id, _memorg, _quad_mode, _need_qe) \
	{ .model = (_model), .nand_id = _nand_id, .memorg = _memorg, \
	  .quad_mode = (_quad_mode), .need_qe = (_need_qe)}

/* Replace the following devices with those don't have parameter pages */
static const struct spi_nand_info spi_nand_flash[] = {
	SPI_NAND_INFO("W25N01GV",
		SPI_NAND_ID(true, 3, 0xef, 0xaa, 0x21, 0x00),
		SPI_NAND_MEMORG_1G_2K_64, true, false),
	SPI_NAND_INFO("MX35LF1GE4AB",
		SPI_NAND_ID(true, 2, 0xc2, 0x12, 0x00, 0x00),
		SPI_NAND_MEMORG_1G_2K_64, true, true)
};

static int spi_nand_reg(bool read_reg, uint8_t reg, uint8_t *val,
			enum spi_mem_data_dir dir)
{
	struct spi_mem_op op;

	zeromem(&op, sizeof(struct spi_mem_op));
	if (read_reg) {
		op.cmd.opcode = SPI_NAND_OP_GET_FEATURE;
	} else {
		op.cmd.opcode = SPI_NAND_OP_SET_FEATURE;
	}

	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.addr.val = reg;
	op.addr.nbytes = 1U;
	op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.data.dir = dir;
	op.data.nbytes = 1U;
	op.data.buf = val;

	return spi_mem_exec_op(&op);
}

static int spi_nand_read_reg(uint8_t reg, uint8_t *val)
{
	return spi_nand_reg(true, reg, val, SPI_MEM_DATA_IN);
}

static int spi_nand_write_reg(uint8_t reg, uint8_t val)
{
	return spi_nand_reg(false, reg, &val, SPI_MEM_DATA_OUT);
}

static int spi_nand_update_cfg(uint8_t mask, uint8_t val)
{
	int ret;
	uint8_t cfg = spinand_dev.cfg_cache;

	cfg &= ~mask;
	cfg |= val;

	if (cfg == spinand_dev.cfg_cache) {
		return 0;
	}

	ret = spi_nand_write_reg(SPI_NAND_REG_CFG, cfg);
	if (ret == 0) {
		spinand_dev.cfg_cache = cfg;
	}

	return ret;
}

static int spi_nand_ecc_enable(bool enable)
{
	return spi_nand_update_cfg(SPI_NAND_CFG_ECC_EN,
				   enable ? SPI_NAND_CFG_ECC_EN : 0U);
}

static int spi_nand_quad_enable(uint8_t manufacturer_id)
{
	bool enable = false;

	if (manufacturer_id != MACRONIX_ID &&
	    manufacturer_id != GIGADEVICE_ID &&
	    manufacturer_id != ETRON_ID &&
	    manufacturer_id != FORESEE_ID) {
		return 0;
	}

	if (spinand_dev.spi_read_cache_op.data.buswidth ==
	    SPI_MEM_BUSWIDTH_4_LINE) {
		enable = true;
	}

	return spi_nand_update_cfg(SPI_NAND_CFG_QE,
				   enable ? SPI_NAND_CFG_QE : 0U);
}

static int spi_nand_wait_ready(uint8_t *status)
{
	int ret;
	uint64_t timeout = timeout_init_us(DELAY_US_400MS);

	while (!timeout_elapsed(timeout)) {
		ret = spi_nand_read_reg(SPI_NAND_REG_STATUS, status);
		if (ret != 0) {
			return ret;
		}

		VERBOSE("%s Status %x\n", __func__, *status);
		if ((*status & SPI_NAND_STATUS_BUSY) == 0U) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

static int spi_nand_reset(void)
{
	struct spi_mem_op op;
	uint8_t status = 0;
	int ret;

	zeromem(&op, sizeof(struct spi_mem_op));
	op.cmd.opcode = SPI_NAND_OP_RESET;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;

	ret = spi_mem_exec_op(&op);
	if (ret != 0) {
		return ret;
	}

	return spi_nand_wait_ready(&status);
}

static int spi_nand_read_id(uint8_t *id)
{
	struct spi_mem_op op;

	zeromem(&op, sizeof(struct spi_mem_op));
	op.cmd.opcode = SPI_NAND_OP_READ_ID;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.data.dir = SPI_MEM_DATA_IN;
	op.data.nbytes = SPI_NAND_MAX_ID_LEN;
	op.data.buf = id;
	op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;

	return spi_mem_exec_op(&op);
}

static int spi_nand_load_page(unsigned int page)
{
	struct spi_mem_op op;
	uint32_t nbpages_per_block = spinand_dev.nand_dev->block_size /
				     spinand_dev.nand_dev->page_size;
	uint32_t block_nb = page / nbpages_per_block;
	uint32_t page_nb = page - (block_nb * nbpages_per_block);
	uint32_t block_sh = __builtin_ctz(nbpages_per_block);

	zeromem(&op, sizeof(struct spi_mem_op));
	op.cmd.opcode = SPI_NAND_OP_LOAD_PAGE;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.addr.val = (block_nb << block_sh) | page_nb;
	op.addr.nbytes = 3U;
	op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;

	return spi_mem_exec_op(&op);
}

static int spi_nand_read_from_cache(unsigned int page, unsigned int offset,
				    uint8_t *buffer, unsigned int len)
{
	uint32_t nbpages_per_block = spinand_dev.nand_dev->block_size /
				     spinand_dev.nand_dev->page_size;
	uint32_t block_nb = page / nbpages_per_block;
	uint32_t page_sh = __builtin_ctz(spinand_dev.nand_dev->page_size) + 1U;

	spinand_dev.spi_read_cache_op.addr.val = offset;

	if ((spinand_dev.nand_dev->nb_planes > 1U) && ((block_nb % 2U) == 1U)) {
		spinand_dev.spi_read_cache_op.addr.val |= (uint64_t)1U << page_sh;
	}

	spinand_dev.spi_read_cache_op.data.buf = buffer;
	spinand_dev.spi_read_cache_op.data.nbytes = len;

	return spi_mem_exec_op(&spinand_dev.spi_read_cache_op);
}

static int spi_nand_read_page(unsigned int page, unsigned int offset,
			      uint8_t *buffer, unsigned int len,
			      bool ecc_enabled)
{
	uint8_t status = 0;
	int ret;

	ret = spi_nand_ecc_enable(ecc_enabled);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_load_page(page);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_wait_ready(&status);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_read_from_cache(page, offset, buffer, len);
	if (ret != 0) {
		return ret;
	}

	if (ecc_enabled && ((status & SPI_NAND_STATUS_ECC_UNCOR) != 0U)) {
		return -EBADMSG;
	}

	return 0;
}

static int spi_nand_mtd_block_is_bad(unsigned int block)
{
	unsigned int nbpages_per_block = spinand_dev.nand_dev->block_size /
					 spinand_dev.nand_dev->page_size;
	uint8_t bbm_marker[2];
	int ret;

	ret = spi_nand_read_page(block * nbpages_per_block,
				 spinand_dev.nand_dev->page_size,
				 bbm_marker, sizeof(bbm_marker), false);
	if (ret != 0) {
		return ret;
	}

	if ((bbm_marker[0] != GENMASK_32(7, 0)) ||
	    (bbm_marker[1] != GENMASK_32(7, 0))) {
		WARN("Block %u is bad\n", block);
		return 1;
	}

	return 0;
}

static int spi_nand_mtd_read_page(struct nand_device *nand, unsigned int page,
				  uintptr_t buffer)
{
	return spi_nand_read_page(page, 0, (uint8_t *)buffer,
				  spinand_dev.nand_dev->page_size, true);
}

static uint16_t crc16(uint16_t crc, uint8_t const *p, uint32_t len)
{
	uint32_t i;

	while (len--) {
		crc ^= *p++ << 8;
		for (i = 0; i < 8; i++)
			crc = (crc << 1) ^ ((crc & 0x8000) ? 0x8005 : 0);
	}

	return crc;
}

static void spi_nand_bit_wise_majority(const void **srcbufs,
				   unsigned int nsrcbufs,
				   void *dstbuf,
				   unsigned int bufsize)
{
	int i, j, k;

	for (i = 0; i < bufsize; i++) {
		uint8_t val = 0;

		for (j = 0; j < 8; j++) {
			unsigned int cnt = 0;

			for (k = 0; k < nsrcbufs; k++) {
				const uint8_t *srcbuf = srcbufs[k];

				if (srcbuf[i] & BIT(j))
					cnt++;
			}

			if (cnt > nsrcbufs / 2)
				val |= BIT(j);
		}

		((uint8_t *)dstbuf)[i] = val;
	}
}

/* There are 3 copies of parameter pages.
 * 1. Choose one avabilable copy first.
 * 2. If none of the copies is available, try to recover.
 * 3. If recovery failed, try another offset to read parameter page.
 */
static int spi_nand_check_pp(struct parameter_page *pp, uint8_t *sel)
{
	int ret = 0;
	int i;
	uint16_t crc = htole16(pp->integrity_crc);
	uint16_t crc_compute;

	for(i=0; i<PARAMETER_PAGE_COPIES; i++) {
		if(htobe32(pp[i].pp_signature) != ONFI_SIGNATURE &&
		   htobe32(pp[i].pp_signature) != NAND_SIGNATURE) {
			ret = -EINVAL;
			continue;
		}

		crc_compute = crc16(SPINAND_CRC_BASE, (uint8_t *)&pp[i],
				    SPINAND_PARAMETER_CRC_OFS);
		INFO("PP COPY %d CRC read: 0x%x, compute: 0x%x\n",
		     i, crc, crc_compute);

		if (crc != crc_compute) {
			ret = -EBADMSG;
			continue;
		}

		*sel=i;
		ret = 0;
		break;
	}

	if(i == PARAMETER_PAGE_COPIES && ret == -EBADMSG) {
		const void *srcbufs[PARAMETER_PAGE_COPIES];
		int j;

		for (j=0; j<PARAMETER_PAGE_COPIES; j++)
			srcbufs[j] = pp + j;

		NOTICE("Couldn't find a valid ONFI parameter page, try bitwise "
		     "majority to recover it\n");
		spi_nand_bit_wise_majority(srcbufs, PARAMETER_PAGE_COPIES, pp,
				       sizeof(*pp));

		crc_compute = crc16(SPINAND_CRC_BASE, (uint8_t *) pp,
			    SPINAND_PARAMETER_CRC_OFS);
		if (crc_compute != htole16(pp->integrity_crc)) {
			ERROR("Parameter page recovery failed, aborting\n");
			return -EBADMSG;
		}

		NOTICE("Parameter page recovery succeeded\n");
		ret = *sel = 0;
	}

	return ret;
}

static int spi_nand_read_pp(struct parameter_page *pp, uint8_t *sel)
{
	uint8_t status;
	uint8_t cfg_reg;
	int ret, op_ret;
	int i;
	uint8_t pp_offset[3] = {0x0, 0x1, 0x4};

	ret = spi_nand_read_reg(SPI_NAND_REG_CFG, &cfg_reg);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_write_reg(SPI_NAND_REG_CFG, cfg_reg | BIT(6));
	if (ret != 0) {
		return ret;
	}

	for(i=0; i<sizeof(pp_offset)/sizeof(uint8_t); i++) {
		op_ret = spi_nand_load_page(pp_offset[i]);
		if (op_ret != 0) {
			goto out;
		}

		op_ret = spi_nand_wait_ready(&status);
		if (op_ret != 0) {
			goto out;
		}

		op_ret = spi_nand_read_from_cache(0x0, 0x0, (uint8_t *)pp,
					       256 * PARAMETER_PAGE_COPIES);
		if (op_ret != 0) {
			goto out;
		}

		ret = spi_nand_check_pp(pp, sel);
		if (!ret) {
			break;
		}
	}
	if(ret) {
		ERROR("Parameter page check failed\n");
	}

out:
	if (op_ret != 0) {
		ERROR("Parameter page read failed\n");
	}

	ret = spi_nand_write_reg(SPI_NAND_REG_CFG, cfg_reg);
	if (ret != 0) {
		return ret;
	}

	if (op_ret != 0) {
		return op_ret;
	}

	return 0;
}

const struct spi_nand_info *get_spi_nand(uint8_t *id, uint8_t *vendor_id)
{
	struct spi_nand_id nand_id;
	uint32_t i, j, id_dummy = 0;

	for (i = 0; i < ARRAY_SIZE(spi_nand_flash); i++) {
		nand_id = spi_nand_flash[i].nand_id;
		if (nand_id.id_dummy)
			id_dummy = 1;
		for (j = 0; j < nand_id.id_len; j++) {
			if (id[j + id_dummy] != nand_id.id_data[j])
				break;
		}

		if (j == nand_id.id_len) {
			*vendor_id = id[id_dummy + 0];
			NOTICE("Recognized SPI-NAND ID: 0x%x 0x%x 0x%x\n",
				id[id_dummy + 0], id[id_dummy+ 1],
				id[id_dummy + 2]);
			return &spi_nand_flash[i];
		}
	}

	return NULL;
}

int spi_nand_set_data_via_id(struct spinand_device *device, uint8_t *id, uint8_t *vendor_id)
{
	const struct spi_nand_info *nand_info;

	nand_info = get_spi_nand(id, vendor_id);
	if (nand_info) {
		device->nand_dev->page_size = nand_info->memorg.pagesize;

		device->nand_dev->nb_planes = nand_info->memorg.planes_per_die;
		device->nand_dev->block_size =
			nand_info->memorg.pages_per_block *
			nand_info->memorg.pagesize;
		device->nand_dev->size =
			(unsigned long long)(nand_info->memorg.blocks_per_die *
			nand_info->memorg.ndies) *
			device->nand_dev->block_size;

		if (!nand_info->quad_mode) {
			device->spi_read_cache_op.cmd.opcode =
				SPI_NAND_OP_READ_FROM_CACHE;
			device->spi_read_cache_op.data.buswidth =
				SPI_MEM_BUSWIDTH_1_LINE;
		}
		return 0;
	}

	ERROR("Unrecognized SPI-NAND ID: 0x%x 0x%x 0x%x 0x%x\n",
		    id[0], id[1], id[2], id[3]);

	return -EINVAL;
}

int spi_nand_set_data_via_pp(struct spinand_device *device,
			     struct parameter_page *pp)
{
	device->nand_dev->page_size = htole32(pp->page_size);
	device->nand_dev->oob_size = htole32(pp->spare_per_page);
	device->nand_dev->nb_planes = pp->plane_addr_bits + 1;
	device->nand_dev->block_size = htole32(pp->pages_per_block) *
				       htole32(pp->page_size);
	device->nand_dev->size = htole32(pp->blocks_per_lun) *
				 htole32(pp->number_of_lun) *
				 htole32(pp->pages_per_block) *
				 htole32(pp->page_size);

	switch(pp->manufactuere_id) {
		/*
		 * For Toshiba SPI Nand, the OOB per page in parameter page
		 * will be half of real size
		 */
		case TOSHIBA_ID:
			device->nand_dev->oob_size <<= 1;
			break;
		/* Almost all of MXIC NANDs except MX35LF1GE4AB use half of OOB
		 * when internal ECC is on.
		 */
		case MACRONIX_ID:
			if (strncmp(pp->device_model, "MX35LF1GE4AB", 12) != 0)
				device->nand_dev->oob_size /= 2;
			break;
		case MICRON_ID:
			/* 2-plane device */
			if (strncmp(pp->device_model, "MT29F2G01ABAGD", 14) == 0)
				device->nand_dev->nb_planes = 2;
			break;
	}

	NOTICE("SPI_NAND parses attributes from parameter page.\n");

	return 0;
}

int spi_nand_init(unsigned long long *size, unsigned int *erase_size)
{
	uint8_t id[SPI_NAND_MAX_ID_LEN];
	uint8_t vendor_id = 0;
	struct parameter_page pp[PARAMETER_PAGE_COPIES];
	uint8_t sel = 0; /* Parameter page select copy, from 0 to 2 */
	int ret;

	spinand_dev.nand_dev = get_nand_device();
	if (spinand_dev.nand_dev == NULL) {
		return -EINVAL;
	}

	spinand_dev.nand_dev->mtd_block_is_bad = spi_nand_mtd_block_is_bad;
	spinand_dev.nand_dev->mtd_read_page = spi_nand_mtd_read_page;
	spinand_dev.nand_dev->nb_planes = 1;

	spinand_dev.spi_read_cache_op.cmd.opcode = SPI_NAND_OP_READ_FROM_CACHE;
	spinand_dev.spi_read_cache_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	spinand_dev.spi_read_cache_op.addr.nbytes = 2U;
	spinand_dev.spi_read_cache_op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	spinand_dev.spi_read_cache_op.dummy.nbytes = 1U;
	spinand_dev.spi_read_cache_op.dummy.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	spinand_dev.spi_read_cache_op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;

	ret = spi_nand_reset();
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_read_pp(pp, &sel);
	if (ret != 0) {
		ERROR("Parameter page read fail, fallback to read ID.\n");
		ret = spi_nand_read_id(id);
		if (ret != 0)
			return ret;
		spi_nand_set_data_via_id(&spinand_dev, id, &vendor_id);
	} else {
		vendor_id = pp[sel].manufactuere_id;
		spi_nand_set_data_via_pp(&spinand_dev, pp + sel);
	}

	assert((spinand_dev.nand_dev->page_size != 0U) &&
	       (spinand_dev.nand_dev->block_size != 0U) &&
	       (spinand_dev.nand_dev->size != 0U));

	zeromem(&spinand_dev.spi_read_cache_op, sizeof(struct spi_mem_op));
	spinand_dev.spi_read_cache_op.cmd.opcode = SPI_NAND_OP_READ_FROM_CACHE_4X;
	spinand_dev.spi_read_cache_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	spinand_dev.spi_read_cache_op.addr.nbytes = 2U;
	spinand_dev.spi_read_cache_op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	spinand_dev.spi_read_cache_op.dummy.nbytes = 1U;
	spinand_dev.spi_read_cache_op.dummy.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	spinand_dev.spi_read_cache_op.data.buswidth = SPI_MEM_BUSWIDTH_4_LINE;
	spinand_dev.spi_read_cache_op.data.dir = SPI_MEM_DATA_IN;

	ret = spi_nand_read_reg(SPI_NAND_REG_CFG, &spinand_dev.cfg_cache);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nand_quad_enable(vendor_id);
	if (ret != 0) {
		return ret;
	}

	NOTICE("SPI_NAND Detected ID 0x%x\n", vendor_id);

	NOTICE("Page size %i, Block size %i, size %lli\n",
		spinand_dev.nand_dev->page_size,
		spinand_dev.nand_dev->block_size,
		spinand_dev.nand_dev->size);

	*size = spinand_dev.nand_dev->size;
	*erase_size = spinand_dev.nand_dev->block_size;

	return 0;
}
