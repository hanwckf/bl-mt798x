/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (C) 2023 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef IO_UBI_H
#define IO_UBI_H

#include <stdint.h>

/*
 * The struct definition is in drivers/mtd/ubispl/ubispl.h. It does
 * not fit into the BSS due to the large buffer requirement of the
 * upstream fastmap code. So the caller of ubispl_load_volumes needs
 * to hand in a pointer to a free memory area where ubispl will place
 * its data. The area is not required to be initialized.
 */
struct ubi_scan_info;

typedef int (*ubi_is_bad_block)(uint32_t pnum);

typedef int (*ubi_read_flash)(uint32_t pnum, unsigned long offset,
			      unsigned long len, void *dst);

/**
 * struct io_ubi_dev_spec - description structure for fast ubi scan
 * This variable must NOT be defined with const qualifier
 * @ubi:		Pointer to memory space for ubi scan info structure
 * @peb_size:		Physical erase block size
 * @vid_offset:		Offset of the VID header
 * @leb_start:		Start of the logical erase block, i.e. offset of data
 * @peb_count:		Number of physical erase blocks in the UBI FLASH area
 *			aka MTD partition.
 * @peb_offset:		Offset of PEB0 in the UBI FLASH area (aka MTD partition)
 *			to the real start of the FLASH in erase blocks.
 * @fastmap:		Enable fastmap attachment
 * @read:		Read function to access the flash
 */

typedef struct io_ubi_dev_spec {
	struct ubi_scan_info	*ubi;
	uint32_t		peb_size;
	uint32_t		vid_offset;
	uint32_t		leb_start;
	uint32_t		peb_count;
	uint32_t		peb_offset;
	int			fastmap;
	ubi_is_bad_block	is_bad_peb;
	ubi_read_flash		read;

	int			init_done;
} io_ubi_dev_spec_t;

struct io_dev_connector;

int register_io_dev_ubi(const struct io_dev_connector **dev_con);

#endif /* IO_UBI_H */
