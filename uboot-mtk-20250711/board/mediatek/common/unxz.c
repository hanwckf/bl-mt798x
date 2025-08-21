// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 *
 * XZ extractor based on XZ Embedded
 * Please refer to lib/xz/README.txt for more info.
 */

#include <display_options.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <malloc.h>
#include <stdint.h>
#include <stdbool.h>
#include <xz/xz.h>
#include "unxz.h"

#if XZ_INTERNAL_CRC32
static bool xz_crc32_initialized;
void xz_crc32_init(void);
#endif
#ifdef XZ_USE_CRC64
static bool xz_crc64_initialized;
void xz_crc64_init(void);
#endif

void *xz_malloc(size_t size)
{
	return malloc(size);
}

int unxz(const void *in_buf, size_t in_len, size_t *out_len,
	 const void *out_buf, size_t out_buf_size)
{
	struct xz_dec *xz;
	struct xz_buf b;
	enum xz_ret xzret;

#if XZ_INTERNAL_CRC32
	if (!xz_crc32_initialized) {
		xz_crc32_init();
		xz_crc32_initialized = true;
	}
#endif

#ifdef XZ_USE_CRC64
	if (!xz_crc64_initialized) {
		xz_crc64_init();
		xz_crc64_initialized = true;
	}
#endif

	xz = xz_dec_init(XZ_SINGLE, 0);
	if (!xz) {
		debug("ERROR: xz: xz_dec_init() failed\n");
		return UNXZ_FORMAT_FAIL;
	}

	b.in = (const uint8_t *)in_buf;
	b.in_pos = 0;
	b.in_size = in_len;

	b.out = (uint8_t *)out_buf;
	b.out_pos = 0;
	b.out_size = out_buf_size;

	xzret = xz_dec_run(xz, &b);

	if (xzret == XZ_FORMAT_ERROR) {
		return UNXZ_FORMAT_FAIL;
	} else if (xzret == XZ_STREAM_END) {
		xz_dec_end(xz);
		*out_len = b.out_pos;
	} else {
		debug("ERROR: xz: xz_dec_run() failed (err = %u)\n", xzret);
		return UNXZ_DEC_FAIL;
	}

	return UNXZ_OK;
}
