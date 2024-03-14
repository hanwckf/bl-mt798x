/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BL2_PLAT_SETUP_H
#define BL2_PLAT_SETUP_H

#include <stddef.h>
#include <stdint.h>

/* BL2 initcalls */
struct initcall {
	void (*callfn)(void);
};

#define INITCALL(_fn)			{ .callfn = (_fn) }

extern const struct initcall bl2_initcalls[];

/* QSPI buffer */
#define QSPI_BUF_OFFSET			0x40100000
#define QSPI_BUF_SIZE			0x100000

/* Scratch buffer */
#define SCRATCH_BUF_OFFSET		0x40400000
#define SCRATCH_BUF_SIZE		0x400000

/* FIP XZ decompression buffer */
#define FIP_DECOMP_BUF_OFFSET		0x40800000
#define FIP_DECOMP_BUF_SIZE		0x400000

/* Block read buffer */
#define IO_BLOCK_BUF_OFFSET		0x41000000
#define IO_BLOCK_BUF_SIZE		0xe00000

int mtk_mmc_gpt_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec,
			    uintptr_t *bkup_image_spec);
int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec);
void mtk_fip_location(size_t *fip_off, size_t *fip_size);
void mtk_bl2_set_dram_size(size_t size);

/* The following function prototypes are provided by platform's boot device */
int mtk_plat_nor_setup(void);
int mtk_plat_nand_setup(size_t *page_size, size_t *block_size, uint64_t *size);
int mtk_plat_mmc_setup(uint32_t *num_sectors);

void mtk_plat_fip_location(size_t *fip_off, size_t *fip_size);

struct mtk_snand_platdata;
const struct mtk_snand_platdata *mtk_plat_get_snfi_platdata(void);

uint32_t mtk_plat_get_qspi_src_clk(void);

#endif /* BL2_PLAT_SETUP_H */
