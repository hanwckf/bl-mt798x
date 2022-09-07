/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image parsing helper
 */

#ifndef _IMAGE_HELPER_H_
#define _IMAGE_HELPER_H_

#include <stdbool.h>
#include <linux/types.h>

enum owrt_image_type {
	IMAGE_RAW,
	IMAGE_UBI1,
	IMAGE_UBI2,
	IMAGE_TAR,

	__IMAGE_TYPE_MAX
};

enum kernel_header_type {
	HEADER_UNKNOWN,
	HEADER_LEGACY,
	HEADER_FIT,

	__HEADER_TYPE_MAX
};

struct owrt_image_info {
	enum owrt_image_type type;
	enum kernel_header_type header_type;

	u32 kernel_size;
	u32 padding_size;

	union {
		u32 rootfs_size;
		u32 ubi_size;
	};

	u32 marker_size;
};

int parse_image_ubi2_ram(const void *data, size_t size, u32 blocksize,
			 struct owrt_image_info *ii);

int parse_image_ram(const void *data, size_t size, u32 blocksize,
		    struct owrt_image_info *ii);

#endif /* _IMAGE_HELPER_H_ */
