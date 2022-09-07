/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include <common/debug.h>
#include <lib/utils.h>
#include <tf_unxz.h>

#include "xz.h"

#if XZ_INTERNAL_CRC32
static bool xz_crc32_initialized;
#endif
#ifdef XZ_USE_CRC64
static bool xz_crc64_initialized;
#endif

#define XZALLOC_ALIGNMENT	sizeof(void *)

static uintptr_t xzalloc_start;
static uintptr_t xzalloc_end;
static uintptr_t xzalloc_current;

#pragma weak xz_simple_putc

void xz_simple_putc(int ch)
{
}

void xz_simple_puts(const char *s)
{
	while (*s) {
		if (*s == '\n')
			xz_simple_putc('\r');
		xz_simple_putc(*s++);
	}
}

void *xz_malloc(size_t size)
{
	uintptr_t p, p_end;

	p = round_up(xzalloc_current, XZALLOC_ALIGNMENT);
	p_end = p + size;

	if (p_end > xzalloc_end)
		return NULL;

	memset((void *)p, 0, size);

	xzalloc_current = p_end;

	return (void *)p;
}

/*
 * unxz - decompress XZ data
 * @in_buf: source of compressed input. Upon exit, the end of input.
 * @in_len: length of in_buf
 * @out_buf: destination of decompressed output. Upon exit, the end of output.
 * @out_len: length of out_buf
 * @work_buf: workspace
 * @work_len: length of workspace
 */
int unxz(uintptr_t *in_buf, size_t in_len, uintptr_t *out_buf, size_t out_len,
	 uintptr_t work_buf, size_t work_len)
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

	xzalloc_start = work_buf;
	xzalloc_end = work_buf + work_len;
	xzalloc_current = xzalloc_start;

	xz = xz_dec_init(XZ_SINGLE, 0);
	if (!xz) {
#if defined(XZ_SIMPLE_PRINT_ERROR)
		xz_simple_puts("xz: xz_dec_init() failed\n");
#else
		ERROR("xz: xz_dec_init() failed\n");
#endif
		return -ENOMEM;
	}

	b.in = (const uint8_t *)*in_buf;
	b.in_pos = 0;
	b.in_size = in_len;

	b.out = (uint8_t *)*out_buf;
	b.out_pos = 0;
	b.out_size = out_len;

	xzret = xz_dec_run(xz, &b);

	if (xzret == XZ_FORMAT_ERROR) {
		/*
		 * Assume data is not compressed, and copy it directly to
		 * output buffer.
		 */
		memcpy((void *)*out_buf, (void *)*in_buf, in_len);
		*in_buf += in_len;
		*out_buf += in_len;
	} else if (xzret == XZ_STREAM_END) {
		xz_dec_end(xz);
		*in_buf += b.in_pos;
		*out_buf += b.out_pos;
	} else {
#if defined(XZ_SIMPLE_PRINT_ERROR)
		xz_simple_puts("xz: xz_dec_run() failed (err = ");
		xz_simple_putc('0' + xzret);
		xz_simple_puts(")\n");
#else
		ERROR("xz: xz_dec_run() failed (err = %u)\n", xzret);
#endif
		return -EIO;
	}

	return 0;
}
