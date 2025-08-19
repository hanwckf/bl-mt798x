/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * OS-dependent definitions for NAND Mapped-block Management (NMBM)
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _NMBM_OS_H_
#define _NMBM_OS_H_

#include <errno.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

uint32_t nmbm_crc32(uint32_t crc, const void *buf, size_t len);

#ifdef __aarch64__
static inline uint32_t nmbm_lldiv(uint64_t dividend, uint32_t divisor)
{
	return dividend / divisor;
}
#else
uint32_t nmbm_lldiv(uint64_t dividend, uint32_t divisor);
#endif

static inline bool is_power_of_2(unsigned long n)
{
	return (n != 0 && ((n & (n - 1)) == 0));
}

static inline int ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;

	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}

	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}

	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}

	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}

	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}

	return r;
}

#define WATCHDOG_RESET()

#ifndef NMBM_DEFAULT_LOG_LEVEL
#define NMBM_DEFAULT_LOG_LEVEL		1
#endif

#endif /* _NMBM_OS_H_ */
