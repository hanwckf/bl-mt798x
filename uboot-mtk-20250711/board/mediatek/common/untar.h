/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Simple TAR extractor
 */

#ifndef _UNTAR_H_
#define _UNTAR_H_

enum tar_file_type {
	TAR_FT_UNKNOWN = 0,
	TAR_FT_REGULAR,
	TAR_FT_LINK,
	TAR_FT_CHAR,
	TAR_FT_BLOCK,
	TAR_FT_DIRECTORY,
	TAR_FT_FIFO
};

struct tar_parse_ctx {
	const char *head;
	size_t offset;
	size_t size;
	int eof;
};

struct tar_file_record {
	const char *name;
	const void *data;
	size_t size;
	unsigned int mode;
	enum tar_file_type type;
	unsigned int uid;
	unsigned int gid;
	const char *uname;
	const char *gname;
	const char *linkname;
};

int tar_ctx_init(struct tar_parse_ctx *ctx, const void *buff, size_t size);
int tar_ctx_next_file(struct tar_parse_ctx *ctx,
		      struct tar_file_record *file_record);

int tar_header_checksum(const void *buff);

int parse_tar_image(const void *data, size_t size,
		    const void **kernel_data, size_t *kernel_size,
		    const void **rootfs_data, size_t *rootfs_size);

#endif /* _UNTAR_H_ */
