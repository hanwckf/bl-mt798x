/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef _BL2_HELPER_H_
#define _BL2_HELPER_H_

#include <stdint.h>
#include "upgrade_helper.h"

/* 2MB bl2 max size */
#define MAX_BL2_SIZE	0x200000
#define BL2_HDR_SIZE	16

/* SF_BOOT */
#define SPIM_NOR_HDR                                                           \
	{                                                                      \
		0x53, 0x46, 0x5f, 0x42, 0x4f, 0x4f, 0x54, 0x00, 0xff, 0xff,    \
			0xff, 0xff, 0x01, 0x00, 0x00, 0x00                     \
	}

/* SPINAND! */
#define SPIM_NAND_HDR                                                          \
	{                                                                      \
		0x53, 0x50, 0x49, 0x4e, 0x41, 0x4e, 0x44, 0x21, 0x01, 0x00,    \
			0x00, 0x00, 0x10, 0x00, 0x00, 0x00                     \
	}

/* NANDCFG! */
#define SNFI_NAND_HDR                                                          \
	{                                                                      \
		0x4e, 0x41, 0x4e, 0x44, 0x43, 0x46, 0x47, 0x21, 0x01, 0x00,    \
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00                     \
	}

/* EMMC_BOOT */
#define EMMC_HDR                                                               \
	{                                                                      \
		0x45, 0x4d, 0x4d, 0x43, 0x5f, 0x42, 0x4f, 0x4f, 0x54, 0x00,    \
			0xff, 0xff, 0x01, 0x00, 0x00, 0x00                     \
	}

/* SDMMC_BOOT */
#define SD_HDR                                                                 \
	{                                                                      \
		0x53, 0x44, 0x4d, 0x4d, 0x43, 0x5f, 0x42, 0x4f, 0x4f, 0x54,    \
			0x00, 0xff, 0x01, 0x00, 0x00, 0x00                     \
	}

struct bl2_entry {
	const char *compat;
	const u8 header[BL2_HDR_SIZE];
};

int bl2_check_image_data(const void *bl2, ulong bl2_size);

#endif /* _BL2_HELPER_H_ */
