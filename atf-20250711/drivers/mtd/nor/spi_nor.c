/*
 * Copyright (c) 2019-2022, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <stddef.h>

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/spi_nor.h>
#include <lib/utils.h>

#define SR_WIP			BIT(0)	/* Write in progress */
#define CR_QUAD_EN_SPAN		BIT(1)	/* Spansion Quad I/O */
#define SR_QUAD_EN_MX		BIT(6)	/* Macronix Quad I/O */
#define FSR_READY		BIT(7)	/* Device status, 0 = Busy, 1 = Ready */

/* Defined IDs for supported memories */
#define SPANSION_ID		0x01U
#define MACRONIX_ID		0xC2U
#define MICRON_ID		0x2CU

#define BANK_SIZE		0x1000000U

#define SPI_READY_TIMEOUT_US	40000U

static struct nor_device nor_dev;

#pragma weak plat_get_nor_data
int plat_get_nor_data(struct nor_device *device, struct nor_device_info *nor_info)
{
	device->size = nor_info->device_size;
	device->flags = nor_info->flags;

	return 0;
}

static int spi_nor_reg(uint8_t reg, uint8_t *buf, size_t len,
		       enum spi_mem_data_dir dir)
{
	struct spi_mem_op op;

	zeromem(&op, sizeof(struct spi_mem_op));
	op.cmd.opcode = reg;
	op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	op.data.dir = dir;
	op.data.nbytes = len;
	op.data.buf = buf;

	return spi_mem_exec_op(&op);
}

static inline int spi_nor_read_id(uint8_t *id)
{
	return spi_nor_reg(SPI_NOR_OP_READ_ID, id, 3U, SPI_MEM_DATA_IN);
}

static inline int spi_nor_read_cr(uint8_t *cr)
{
	return spi_nor_reg(SPI_NOR_OP_READ_CR, cr, 1U, SPI_MEM_DATA_IN);
}

static inline int spi_nor_read_sr(uint8_t *sr)
{
	return spi_nor_reg(SPI_NOR_OP_READ_SR, sr, 1U, SPI_MEM_DATA_IN);
}

static inline int spi_nor_read_fsr(uint8_t *fsr)
{
	return spi_nor_reg(SPI_NOR_OP_READ_FSR, fsr, 1U, SPI_MEM_DATA_IN);
}

static inline int spi_nor_write_en(void)
{
	return spi_nor_reg(SPI_NOR_OP_WREN, NULL, 0U, SPI_MEM_DATA_OUT);
}

/*
 * Check if device is ready.
 *
 * Return 0 if ready, 1 if busy or a negative error code otherwise
 */
static int spi_nor_ready(void)
{
	uint8_t sr;
	int ret;

	ret = spi_nor_read_sr(&sr);
	if (ret != 0) {
		return ret;
	}

	if ((nor_dev.flags & SPI_NOR_USE_FSR) != 0U) {
		uint8_t fsr;

		ret = spi_nor_read_fsr(&fsr);
		if (ret != 0) {
			return ret;
		}

		return (((fsr & FSR_READY) != 0U) && ((sr & SR_WIP) == 0U)) ?
			0 : 1;
	}

	return (((sr & SR_WIP) == 0U) ? 0 : 1);
}

static int spi_nor_wait_ready(void)
{
	int ret;
	uint64_t timeout = timeout_init_us(SPI_READY_TIMEOUT_US);

	while (!timeout_elapsed(timeout)) {
		ret = spi_nor_ready();
		if (ret <= 0) {
			return ret;
		}
	}

	return -ETIMEDOUT;
}

static int spi_nor_macronix_quad_enable(void)
{
	uint8_t sr;
	int ret;

	ret = spi_nor_read_sr(&sr);
	if (ret != 0) {
		return ret;
	}

	if ((sr & SR_QUAD_EN_MX) != 0U) {
		return 0;
	}

	ret = spi_nor_write_en();
	if (ret != 0) {
		return ret;
	}

	sr |= SR_QUAD_EN_MX;
	ret = spi_nor_reg(SPI_NOR_OP_WRSR, &sr, 1U, SPI_MEM_DATA_OUT);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nor_wait_ready();
	if (ret != 0) {
		return ret;
	}

	ret = spi_nor_read_sr(&sr);
	if ((ret != 0) || ((sr & SR_QUAD_EN_MX) == 0U)) {
		return -EINVAL;
	}

	return 0;
}

static int spi_nor_write_sr_cr(uint8_t *sr_cr)
{
	int ret;

	ret = spi_nor_write_en();
	if (ret != 0) {
		return ret;
	}

	ret = spi_nor_reg(SPI_NOR_OP_WRSR, sr_cr, 2U, SPI_MEM_DATA_OUT);
	if (ret != 0) {
		return -EINVAL;
	}

	ret = spi_nor_wait_ready();
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static int spi_nor_quad_enable(void)
{
	uint8_t sr_cr[2];
	int ret;

	ret = spi_nor_read_cr(&sr_cr[1]);
	if (ret != 0) {
		return ret;
	}

	if ((sr_cr[1] & CR_QUAD_EN_SPAN) != 0U) {
		return 0;
	}

	sr_cr[1] |= CR_QUAD_EN_SPAN;
	ret = spi_nor_read_sr(&sr_cr[0]);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nor_write_sr_cr(sr_cr);
	if (ret != 0) {
		return ret;
	}

	ret = spi_nor_read_cr(&sr_cr[1]);
	if ((ret != 0) || ((sr_cr[1] & CR_QUAD_EN_SPAN) == 0U)) {
		return -EINVAL;
	}

	return 0;
}

static int spi_nor_clean_bar(void)
{
	int ret;

	if (nor_dev.selected_bank == 0U) {
		return 0;
	}

	nor_dev.selected_bank = 0U;

	ret = spi_nor_write_en();
	if (ret != 0) {
		return ret;
	}

	return spi_nor_reg(nor_dev.bank_write_cmd, &nor_dev.selected_bank,
			   1U, SPI_MEM_DATA_OUT);
}

static int spi_nor_write_bar(uint32_t offset)
{
	uint8_t selected_bank = offset / BANK_SIZE;
	int ret;

	if (selected_bank == nor_dev.selected_bank) {
		return 0;
	}

	ret = spi_nor_write_en();
	if (ret != 0) {
		return ret;
	}

	ret = spi_nor_reg(nor_dev.bank_write_cmd, &selected_bank,
			  1U, SPI_MEM_DATA_OUT);
	if (ret != 0) {
		return ret;
	}

	nor_dev.selected_bank = selected_bank;

	return 0;
}

static int spi_nor_read_bar(void)
{
	uint8_t selected_bank = 0U;
	int ret;

	ret = spi_nor_reg(nor_dev.bank_read_cmd, &selected_bank,
			  1U, SPI_MEM_DATA_IN);
	if (ret != 0) {
		return ret;
	}

	nor_dev.selected_bank = selected_bank;

	return 0;
}

int spi_nor_read(unsigned int offset, uintptr_t buffer, size_t length,
		 size_t *length_read)
{
	size_t remain_len;
	int ret;

	*length_read = 0U;
	nor_dev.read_op.addr.val = offset;
	nor_dev.read_op.data.buf = (void *)buffer;

	VERBOSE("%s offset %u length %zu\n", __func__, offset, length);

	while (length != 0U) {
		if ((nor_dev.flags & SPI_NOR_USE_BANK) != 0U) {
			ret = spi_nor_write_bar(nor_dev.read_op.addr.val);
			if (ret != 0) {
				return ret;
			}

			remain_len = (BANK_SIZE * (nor_dev.selected_bank + 1)) -
				nor_dev.read_op.addr.val;
			nor_dev.read_op.data.nbytes = MIN(length, remain_len);
		} else {
			nor_dev.read_op.data.nbytes = length;
		}

		ret = spi_mem_exec_op(&nor_dev.read_op);
		if (ret != 0) {
			spi_nor_clean_bar();
			return ret;
		}

		length -= nor_dev.read_op.data.nbytes;
		nor_dev.read_op.addr.val += nor_dev.read_op.data.nbytes;
		nor_dev.read_op.data.buf += nor_dev.read_op.data.nbytes;
		*length_read += nor_dev.read_op.data.nbytes;
	}

	if ((nor_dev.flags & SPI_NOR_USE_BANK) != 0U) {
		ret = spi_nor_clean_bar();
		if (ret != 0) {
			return ret;
		}
	}

	return 0;
}

struct nor_device_info nor_flash_info_table[] = {
	{"EN25Q128B", {0x1C, 0x30, 0x18}, 0x1000000, 0},
	{"EN25QH128", {0x1C, 0x70, 0x18}, 0x1000000, 0},
	{"EN25QX128A", {0x1C, 0x71, 0x18}, 0x1000000, 0},
	{"EN25QH256", {0x1C, 0x70, 0x19}, 0x2000000, 0},
	{"EN25QX256A", {0x1C, 0x71, 0x19}, 0x2000000, 0},

	{"F25L128QA", {0x8C, 0x41, 0x18}, 0x1000000, 0},

	{"GD25Q128", {0xC8, 0x40, 0x18}, 0x1000000, 0},
	{"GD25Q256", {0xC8, 0x40, 0x19}, 0x2000000, 0},
	{"GD25Q512", {0xC8, 0x40, 0x20}, 0x4000000, 0},
	{"GD55F512MF", {0xC8, 0x43, 0x1A}, 0x4000000, 0},
	{"GD25T512ME", {0xC8, 0x46, 0x1A}, 0x4000000, 0},
	{"GD25B512ME", {0xC8, 0x47, 0x1A}, 0x4000000, 0},
	{"GD55T01GE", {0xC8, 0x46, 0x1B}, 0x8000000, 0},
	{"GD55B01GE", {0xC8, 0x47, 0x1B}, 0x8000000, 0},
	{"GD55T02GE", {0xC8, 0x46, 0x1C}, 0x10000000, 0},
	{"GD55B02GE", {0xC8, 0x47, 0x1C}, 0x10000000, 0},

	{"IS25LP128", {0x9D, 0x60, 0x18}, 0x1000000, 0},
	{"IS25LP256", {0x9D, 0x60, 0x19}, 0x2000000, 0},
	{"IS25LP512", {0x9D, 0x60, 0x1A}, 0x4000000, 0},
	{"IS25LP01G", {0x9D, 0x60, 0x1B}, 0x8000000, 0},

	{"MX25L12805D", {0xC2, 0x20, 0x18}, 0x1000000, 0},
	{"MX25L12833F", {0xC2, 0x20, 0x18}, 0x1000000, 0},
	{"MX25L12855E", {0xC2, 0x26, 0x18}, 0x1000000, 0},
	{"MX25L25635E", {0xC2, 0x20, 0x19}, 0x2000000, 0},
	{"MX25L25645G", {0xC2, 0x20, 0x19}, 0x2000000, 0},
	{"MX25L25655E", {0xC2, 0x26, 0x19}, 0x2000000, 0},
	{"MX25LM25645G", {0xC2, 0x85, 0x39}, 0x2000000, 0},
	{"MX25L51245G", {0xC2, 0x20, 0x1A}, 0x4000000, 0},
	{"MX25LM51245G", {0xC2, 0x85, 0x3A}, 0x4000000, 0},
	{"MX25LW51245G", {0xC2, 0x86, 0x3A}, 0x4000000, 0},
	{"MX66L1G45G", {0xC2, 0x20, 0x1B}, 0x8000000, 0},
	{"MX66LM1G45G", {0xC2, 0x85, 0x3B}, 0x8000000, 0},
	{"MX66L2G45G", {0xC2, 0x20, 0x1C}, 0x10000000, 0},

	{"N25Q128A13", {0x20, 0xBA, 0x18}, 0x1000000, 0},
	{"N25Q256A", {0x20, 0xBA, 0x19}, 0x2000000, 0},
	{"N25Q512Ax3", {0x20, 0xBA, 0x20}, 0x4000000, SPI_NOR_USE_FSR},
	{"N25Q00", {0x20, 0xBA, 0x21}, 0x8000000, SPI_NOR_USE_FSR},

	{"MT25QL01G", {0x21, 0xBA, 0x20}, 0x8000000, SPI_NOR_USE_FSR},
	{"MT25QL02G", {0x20, 0xBA, 0x22}, 0x10000000, SPI_NOR_USE_FSR},

	{"W25Q128xV", {0xEF, 0x40, 0x18}, 0x1000000, 0},
	{"W25Q128JV", {0xEF, 0x70, 0x18}, 0x1000000, 0},
	{"W25Q256xV", {0xEF, 0x40, 0x19}, 0x2000000, 0},
	{"W25Q256JV", {0xEF, 0x70, 0x19}, 0x2000000, 0},
	{"W25Q512xV", {0xEF, 0x40, 0x20}, 0x4000000, 0},
	{"W25Q512JV", {0xEF, 0x70, 0x20}, 0x4000000, 0},
	{"W25M512JV", {0xEF, 0x71, 0x20}, 0x4000000, 0},
	{"W25Q01JV", {0xEF, 0x40, 0x21}, 0x8000000, 0},
	{"W25H02JV", {0xEF, 0x90, 0x22}, 0x10000000, 0},

	{"XM25QH128C", {0x20, 0x40, 0x18}, 0x1000000, 0},
	{"XM25QW128C", {0x20, 0x42, 0x18}, 0x1000000, 0},
	{"XM25QH128A", {0x20, 0x70, 0x18}, 0x1000000, 0},
	{"XM25QW256C", {0x20, 0x42, 0x19}, 0x2000000, 0},
	{"XM25QH256B", {0x20, 0x60, 0x19}, 0x2000000, 0},
	{"XM25QU256B", {0x20, 0x70, 0x19}, 0x2000000, 0},

	{"XT25F128B", {0x0B, 0x40, 0x18}, 0x1000000, 0},
	{"XT25F256B", {0x0B, 0x40, 0x19}, 0x2000000, 0},

	{"P25Q128", {0x85, 0x60, 0x18}, 0x1000000, 0},

	{"ZB25VQ128", {0x5E, 0x40, 0x18}, 0x1000000, 0},

	{"FM25Q128", {0xA1, 0x40, 0x18}, 0x1000000, 0},
};


struct nor_device_info * get_flash_info(uint8_t *id)
{
	uint8_t	idx, j;
	uint8_t	nor_info_table_size = ARRAY_SIZE(nor_flash_info_table);

	for (idx = 0; idx < nor_info_table_size; ++idx) {
		for (j = 0; j < FLASH_DEVICE_ID_LENGTH; ++j) {
			if (nor_flash_info_table[idx].device_id[j] != id[j]) {
				break;
			}
		}

		if (j == FLASH_DEVICE_ID_LENGTH) {
			return (nor_flash_info_table + idx);
		}
	}

	return NULL;
}


int spi_nor_init(unsigned long long *size, unsigned int *erase_size)
{
	int ret = 0;
	uint8_t id[FLASH_DEVICE_ID_LENGTH] = {0, 0, 0};
	struct nor_device_info *nor_info = NULL;

	/* Default read command used */
	nor_dev.read_op.cmd.opcode = SPI_NOR_OP_READ;
	nor_dev.read_op.cmd.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	nor_dev.read_op.addr.nbytes = 3U;
	nor_dev.read_op.addr.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	nor_dev.read_op.data.buswidth = SPI_MEM_BUSWIDTH_1_LINE;
	nor_dev.read_op.data.dir = SPI_MEM_DATA_IN;

	ret = spi_nor_read_id(id);
	if (ret != 0) {
		return ret;
	}

	nor_info = get_flash_info(id);
	if (nor_info == NULL) {
		ERROR("No Flash Device Matched\n");
		return -EINVAL;
	}

	if (plat_get_nor_data(&nor_dev, nor_info) != 0) {
		return -EINVAL;
	}

	assert(nor_dev.size != 0U);

	/* flash size bigger than 16MB, use extend address register */
	if (nor_dev.size > BANK_SIZE) {
		nor_dev.flags |= SPI_NOR_USE_BANK;
	}

	*size = nor_dev.size;

	if ((nor_dev.flags & SPI_NOR_USE_BANK) != 0U) {
		switch (id[0]) {
		case SPANSION_ID:
			nor_dev.bank_read_cmd = SPINOR_OP_BRRD;
			nor_dev.bank_write_cmd = SPINOR_OP_BRWR;
			break;
		default:
			nor_dev.bank_read_cmd = SPINOR_OP_RDEAR;
			nor_dev.bank_write_cmd = SPINOR_OP_WREAR;
			break;
		}
	}

	if (nor_dev.read_op.data.buswidth == 4U) {
		switch (id[0]) {
		case MACRONIX_ID:
			ret = spi_nor_macronix_quad_enable();
			break;
		case MICRON_ID:
			break;
		default:
			ret = spi_nor_quad_enable();
			break;
		}
	}

	if ((ret == 0) && ((nor_dev.flags & SPI_NOR_USE_BANK) != 0U)) {
		ret = spi_nor_read_bar();
	}

	return ret;
}

