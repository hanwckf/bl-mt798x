/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef FIP_H
#define FIP_H

#include <uuid.h>
#include <stdint.h>
#include <linux/list.h>

#define MAX_TOE_ENTRY	50

/* Length of a node address (an IEEE 802 address). */
#define _UUID_NODE_LEN	6

/* Length of UUID string including dashes. */
#define _UUID_STR_LEN	36

struct atf_uuid {
	u8 time_low[4];
	u8 time_mid[2];
	u8 time_hi_and_version[2];
	u8 clock_seq_hi_and_reserved;
	u8 clock_seq_low;
	u8 node[_UUID_NODE_LEN];
} __packed;

union fip_uuid {
	struct atf_uuid atf_uuid;
	struct uuid uboot_uuid;
};

struct fip_toc_header {
	u32 name;
	u32 serial_number;
	u64 flags;
};

struct fip_toc_entry {
	union fip_uuid uuid;
	u64 offset_address;
	u64 size;
	u64 flags;
};

struct image_desc {
	union fip_uuid uuid;
	const char *desc;
	const char *name;
	const void *data;
	struct fip_toc_entry toc_e;
	struct list_head list;
};

struct fip {
	struct fip_toc_header *toc_header;
	struct list_head images;
	const void *data;
	size_t size;
};

/*****************************************************************************
 * TOC ENTRIES
 *****************************************************************************/
struct toc_entry {
	const char *desc;
	union fip_uuid uuid;
	const char *name;
};

int init_fip(const void *data, size_t size, struct fip *fip);
void free_fip(struct fip *fip);
int find_image_desc(struct fip *fip, const char *name,
		    struct image_desc **image_desc);
int fip_show_image(struct fip *fip, const char *name);
int fip_get_image(struct fip *fip, const char *name, const void **data,
		  size_t *size);
int fip_show(struct fip *fip);
int fip_unpack(struct fip *fip, const char *name, void *data_out,
	       size_t max_len, size_t *data_len_out);
int fip_update(struct fip *fip, const char *name, const void *data_in,
	       size_t len);
int fip_create(struct fip *fip, void *data_out, size_t max_len,
	       size_t *data_len_out);
int fip_repack(struct fip *fip, void *data_swap, size_t max_len);

#endif /* FIP_H */
