/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef DRIVERS_MTK_SPI_NAND_H
#define DRIVERS_MTK_SPI_NAND_H

#include <drivers/spi_nand.h>

#define SPI_NAND_MAX_ID_LEN             4U

/* For parameter page CRC */
#define ONFI_CRC_BASE		(0x4F4E)
#define CASN_CRC_BASE		(0x4341)
#define SPINAND_PARAMETER_CRC_OFS	(254)
#define SPINAND_CASN_CRC_OFS		(254)

#define SPINAND_HAS_QE_BIT			BIT(0)
#define SPINAND_HAS_CR_FEAT_BIT			BIT(1)
#define SPINAND_SUP_CR				BIT(2)
#define SPINAND_SUP_ON_DIE_ECC			BIT(3)
#define SPINAND_SUP_LEGACY_ECC_STATUS		BIT(4)
#define SPINAND_SUP_ADV_ECC_STATUS		BIT(5)
#define SPINAND_ECC_PARITY_READABLE		BIT(6)

struct op_slice {
	uint8_t cmd_opcode;
#if _BYTE_ORDER == _LITTLE_ENDIAN
	uint8_t dummy_nbytes : 4;
	uint8_t addr_nbytes : 4;
#else
	uint8_t addr_nbytes : 4;
	uint8_t dummy_nbytes : 4;
#endif
};

struct SPINAND_FLAGS {
#if _BYTE_ORDER == _LITTLE_ENDIAN
	uint8_t has_qe_bit : 1;
	uint8_t has_cr_feat_bit : 1;
	uint8_t conti_read_cap : 1;
	uint8_t on_die_ecc : 1;
	uint8_t legacy_ecc_status : 1;
	uint8_t adv_ecc_status : 1;
	uint8_t ecc_parity_readable : 1;
	uint8_t ecc_alg : 1; /* ECC algorithm */
#else
	uint8_t ecc_alg : 1; /* ECC algorithm */
	uint8_t ecc_parity_readable : 1;
	uint8_t adv_ecc_status : 1;
	uint8_t legacy_ecc_status : 1;
	uint8_t on_die_ecc : 1;
	uint8_t conti_read_cap : 1;
	uint8_t has_cr_feat_bit : 1;
	uint8_t has_qe_bit : 1;
#endif
};

struct ADV_ECC_STATUS {
	uint8_t cmd;
	uint8_t addr;
	uint8_t addr_nbytes;
	uint8_t addr_buswidth;
	uint8_t dummy_nbytes;
	uint8_t dummy_buswidth;
	uint8_t status_nbytes;
	uint16_t status_mask;
	uint8_t pre_op; /* pre-process operator */
	uint8_t pre_mask; /* pre-process mask */
}__attribute__((packed));

struct CASN_OOB {
	uint8_t layout_type;

	/* OOB free layout */
	uint8_t free_start;
	uint8_t free_length;
	uint8_t bbm_length;

	/* ECC parity layout */
	uint8_t ecc_parity_start;
	uint8_t ecc_parity_space;
	uint8_t ecc_parity_real_length;
};

enum oob_overall {
	OOB_DISCRETE = 0,
	OOB_CONTINUOUS,
};

struct casn_page {
	/* CASN signature must be 4 chars: 'C','A','S','N'  */
	uint32_t signature;

	uint8_t version;
	char manufacturer[13];
	char model[16];

	uint32_t bits_per_cell;
	uint32_t bytes_per_page;
	uint32_t spare_bytes_per_page;
	uint32_t pages_per_block;
	uint32_t blocks_per_lun;
	uint32_t max_bb_per_lun;
	uint32_t planes_per_lun;
	uint32_t luns_per_target;
	uint32_t total_target;

	uint32_t ecc_strength;
	uint32_t ecc_step_size;

	uint8_t flags;
	uint8_t reserved1;

	uint16_t sdr_read_cap;
	struct op_slice sdr_read_1_1_1;
	struct op_slice sdr_read_1_1_1_fast;
	struct op_slice sdr_read_1_1_2;
	struct op_slice sdr_read_1_2_2;
	struct op_slice sdr_read_1_1_4;
	struct op_slice sdr_read_1_4_4;
	struct op_slice sdr_read_1_1_8;
	struct op_slice sdr_read_1_8_8;

	struct op_slice sdr_cont_read_1_1_1;
	struct op_slice sdr_cont_read_1_1_1_fast;
	struct op_slice sdr_cont_read_1_1_2;
	struct op_slice sdr_cont_read_1_2_2;
	struct op_slice sdr_cont_read_1_1_4;
	struct op_slice sdr_cont_read_1_4_4;
	struct op_slice sdr_cont_read_1_1_8;
	struct op_slice sdr_cont_read_1_8_8;

	uint16_t ddr_read_cap;
	struct op_slice ddr_read_1_1_1;
	struct op_slice ddr_read_1_1_1_fast;
	struct op_slice ddr_read_1_1_2;
	struct op_slice ddr_read_1_2_2;
	struct op_slice ddr_read_1_1_4;
	struct op_slice ddr_read_1_4_4;
	struct op_slice ddr_read_1_1_8;
	struct op_slice ddr_read_1_8_8;

	struct op_slice ddr_cont_read_1_1_1;
	struct op_slice ddr_cont_read_1_1_1_fast;
	struct op_slice ddr_cont_read_1_1_2;
	struct op_slice ddr_cont_read_1_2_2;
	struct op_slice ddr_cont_read_1_1_4;
	struct op_slice ddr_cont_read_1_4_4;
	struct op_slice ddr_cont_read_1_1_8;
	struct op_slice ddr_cont_read_1_8_8;

	uint8_t sdr_write_cap;
	struct op_slice sdr_write_1_1_1;
	struct op_slice sdr_write_1_1_4;
	struct op_slice reserved2[6];
	uint8_t ddr_write_cap;
	struct op_slice reserved3[8];

	uint8_t sdr_update_cap;
	struct op_slice sdr_update_1_1_1;
	struct op_slice sdr_update_1_1_4;
	struct op_slice reserved4[6];
	uint8_t ddr_update_cap;
	struct op_slice reserved5[8];

	struct CASN_OOB casn_oob;

	/* Advanced ECC status CMD0 (higher bits) */
	struct ADV_ECC_STATUS ecc_status_high;
	/* Advanced ECC status CMD1 (lower bits) */
	struct ADV_ECC_STATUS ecc_status_low;

	uint8_t advecc_noerr_status;
	uint8_t advecc_uncor_status;
	uint8_t advecc_post_op;
	uint8_t advecc_post_mask;

	uint8_t reserved6[5];
	uint16_t crc;
}__attribute__((packed));

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

#endif /* DRIVERS_MTK_SPI_NAND_H */
