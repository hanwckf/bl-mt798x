/*
 * Copyright (c) 2019, STMicroelectronics - All Rights Reserved
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DRIVERS_SPI_NAND_H
#define DRIVERS_SPI_NAND_H

#include <drivers/nand.h>
#include <drivers/spi_mem.h>

#define SPI_NAND_MAX_ID_LEN		4U

#define SPI_NAND_OP_GET_FEATURE		0x0FU
#define SPI_NAND_OP_SET_FEATURE		0x1FU
#define SPI_NAND_OP_READ_ID		0x9FU
#define SPI_NAND_OP_LOAD_PAGE		0x13U
#define SPI_NAND_OP_RESET		0xFFU
#define SPI_NAND_OP_READ_FROM_CACHE	0x03U
#define SPI_NAND_OP_READ_FROM_CACHE_2X	0x3BU
#define SPI_NAND_OP_READ_FROM_CACHE_4X	0x6BU

/* Configuration register */
#define SPI_NAND_REG_CFG		0xB0U
#define SPI_NAND_CFG_ECC_EN		BIT(4)
#define SPI_NAND_CFG_QE			BIT(0)

/* Status register */
#define SPI_NAND_REG_STATUS		0xC0U
#define SPI_NAND_STATUS_BUSY		BIT(0)
#define SPI_NAND_STATUS_ECC_UNCOR	BIT(5)

/* For parameter page CRC */
#define SPINAND_CRC_BASE		(0x4F4E)
#define SPINAND_PARAMETER_CRC_OFS	(254)

struct parameter_page {
	uint32_t pp_signature;
	uint16_t revision;
	uint16_t feature_sup;
	uint16_t option_cmd_sup;
	uint8_t reserved_1[22];
	uint8_t device_manufacturer[12];
	char device_model[20];
	uint8_t manufactuere_id;
	uint16_t date_code;
	uint8_t reserved_2[13];
	uint32_t page_size;
	uint16_t spare_per_page;
	uint32_t partial_page_size;
	uint16_t spare_per_partial_page;
	uint32_t pages_per_block;
	uint32_t blocks_per_lun;
	/* byte 100 */
	uint8_t number_of_lun;
	uint8_t number_of_addr_bytes;
	uint8_t bits_per_cell;
	uint16_t max_bad_blocks;
	uint16_t be; /* block endurance */
	uint8_t guar_vb; /* guaranteed valid blocks at beginning */
	uint16_t be_for_guar_vb;
	uint8_t prog_per_page; /* programs per page */
	uint8_t partial_prog_attr;
	uint8_t ecc_cap; /* ECC correctability (bits) */
	uint8_t plane_addr_bits;
	uint8_t multi_plane_op_attr;
	/* byte 115 */
	uint8_t reserved_3[13];
	uint8_t io_capacitance;
	uint16_t io_clk_sup;
	uint8_t reserved[2];
	uint16_t max_tPROG;
	uint16_t max_tBERS;
	uint16_t max_tR;
	uint8_t reserved_4[25];
	uint16_t vendor_resivion;
	uint8_t vendor_specific[88];
	uint16_t integrity_crc;
}__attribute__((packed));

struct spinand_device {
	struct nand_device *nand_dev;
	struct spi_mem_op spi_read_cache_op;
	uint8_t cfg_cache; /* Cached value of SPI NAND device register CFG */
};

struct device_mem_org {
	uint16_t pagesize;
	uint16_t sparesize;
	uint16_t pages_per_block;
	uint16_t blocks_per_die;
	uint16_t planes_per_die;
	uint16_t ndies;
};

struct spi_nand_id {
	bool id_dummy;
	uint8_t id_len;
	uint8_t id_data[SPI_NAND_MAX_ID_LEN];
};

struct spi_nand_info {
	const char *model;
	const struct spi_nand_id nand_id;
	const struct device_mem_org memorg;
	bool quad_mode;
	bool need_qe;
};

int spi_nand_init(unsigned long long *size, unsigned int *erase_size);

#endif /* DRIVERS_SPI_NAND_H */
