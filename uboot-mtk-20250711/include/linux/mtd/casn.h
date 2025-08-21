// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: SkyLake.Huang <skylake.huang@mediatek.com>
 */

#ifndef __LINUX_MTD_CASN_H
#define __LINUX_MTD_CASN_H

#define CASN_CRC_BASE	0x4341
#define CASN_SIGNATURE	0x4341534EU
#define SPINAND_CASN_V1_CRC_OFS (254)
#define CASN_PAGE_V1_COPIES     (3)

#define SDR_READ_1_1_1		BIT(0)
#define SDR_READ_1_1_1_FAST	BIT(1)
#define SDR_READ_1_1_2		BIT(2)
#define SDR_READ_1_2_2		BIT(3)
#define SDR_READ_1_1_4		BIT(4)
#define SDR_READ_1_4_4		BIT(5)
#define SDR_READ_1_1_8		BIT(6)
#define SDR_READ_1_8_8		BIT(7)

#define SDR_WRITE_1_1_1		BIT(0)
#define SDR_WRITE_1_1_4		BIT(1)

#define SDR_UPDATE_1_1_1	BIT(0)
#define SDR_UPDATE_1_1_4	BIT(1)

struct op_slice {
	u8 cmd_opcode;
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u8 dummy_nbytes : 4;
	u8 addr_nbytes : 4;
#elif defined(__BIG_ENDIAN_BITFIELD)
	u8 addr_nbytes : 4;
	u8 dummy_nbytes : 4;
#endif
};

struct SPINAND_FLAGS {
#if defined(__LITTLE_ENDIAN_BITFIELD)
	u8 has_qe_bit : 1;
	u8 has_cr_feat_bit : 1;
	u8 conti_read_cap : 1;
	u8 on_die_ecc : 1;
	u8 legacy_ecc_status : 1;
	u8 adv_ecc_status : 1;
	u8 ecc_parity_readable : 1;
	u8 ecc_alg : 1; /* ECC algorithm */
#elif defined(__BIG_ENDIAN_BITFIELD)
	u8 ecc_alg : 1; /* ECC algorithm */
	u8 ecc_parity_readable : 1;
	u8 adv_ecc_status : 1;
	u8 legacy_ecc_status : 1;
	u8 on_die_ecc : 1;
	u8 conti_read_cap : 1;
	u8 has_cr_feat_bit : 1;
	u8 has_qe_bit : 1;
#endif
};

struct ADV_ECC_STATUS {
	u8 cmd;
	u8 addr;
	u8 addr_nbytes;
	u8 addr_buswidth;
	u8 dummy_nbytes;
	u8 dummy_buswidth;
	u8 status_nbytes;
	u16 status_mask;
	u8 pre_op; /* pre-process operator */
	u8 pre_mask; /* pre-process mask */
} __packed;

struct CASN_OOB {
	u8 layout_type;

	/* OOB free layout */
	u8 free_start;
	u8 free_length;
	u8 bbm_length;

	/* ECC parity layout */
	u8 ecc_parity_start;
	u8 ecc_parity_space;
	u8 ecc_parity_real_length;
};

enum oob_overall {
	OOB_DISCRETE = 0,
	OOB_CONTINUOUS,
};

struct nand_casn {
	/* CASN signature must be 4 chars: 'C','A','S','N'  */
	union {
		u8 sig[4];
		u32 signature;
	};

	u8 version;
	char manufacturer[13];
	char model[16];

	__be32 bits_per_cell;
	__be32 bytes_per_page;
	__be32 spare_bytes_per_page;
	__be32 pages_per_block;
	__be32 blocks_per_lun;
	__be32 max_bb_per_lun;
	__be32 planes_per_lun;
	__be32 luns_per_target;
	__be32 total_target;

	__be32 ecc_strength;
	__be32 ecc_step_size;

	u8 flags;
	u8 reserved1;

	__be16 sdr_read_cap;
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

	__be16 ddr_read_cap;
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

	u8 sdr_write_cap;
	struct op_slice sdr_write_1_1_1;
	struct op_slice sdr_write_1_1_4;
	struct op_slice reserved2[6];
	u8 ddr_write_cap;
	struct op_slice reserved3[8];

	u8 sdr_update_cap;
	struct op_slice sdr_update_1_1_1;
	struct op_slice sdr_update_1_1_4;
	struct op_slice reserved4[6];
	u8 ddr_update_cap;
	struct op_slice reserved5[8];

	struct CASN_OOB casn_oob;

	/* Advanced ECC status CMD0 (higher bits) */
	struct ADV_ECC_STATUS ecc_status_high;
	/* Advanced ECC status CMD1 (lower bits) */
	struct ADV_ECC_STATUS ecc_status_low;

	u8 advecc_noerr_status;
	u8 advecc_uncor_status;
	u8 advecc_post_op;
	u8 advecc_post_mask;

	u8 reserved6[5];
	__be16 crc;
} __packed;

#endif /* __LINUX_MTD_CASN_H */
