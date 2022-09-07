/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * OS-dependent definitions for NAND Mapped-block Management (NMBM)
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _NMBM_OS_H_
#define _NMBM_OS_H_

#include <div64.h>
#include <stdbool.h>
#include <watchdog.h>
#include <u-boot/crc.h>
#include <linux/errno.h>
#include <linux/log2.h>
#include <linux/types.h>

static inline uint32_t nmbm_crc32(uint32_t crcval, const void *buf, size_t size)
{
	uint chksz;
	const unsigned char *p = buf;

	while (size) {
		if (size > UINT_MAX)
			chksz = UINT_MAX;
		else
			chksz = (uint)size;

		crcval = crc32_no_comp(crcval, p, chksz);
		size -= chksz;
		p += chksz;
	}

	return crcval;
}

static inline uint32_t nmbm_lldiv(uint64_t dividend, uint32_t divisor)
{
#if BITS_PER_LONG == 64
	return dividend / divisor;
#else
	__div64_32(&dividend, divisor);
	return dividend;
#endif
}

#ifdef CONFIG_NMBM_LOG_LEVEL_DEBUG
#define NMBM_DEFAULT_LOG_LEVEL		0
#elif defined(NMBM_LOG_LEVEL_INFO)
#define NMBM_DEFAULT_LOG_LEVEL		1
#elif defined(NMBM_LOG_LEVEL_WARN)
#define NMBM_DEFAULT_LOG_LEVEL		2
#elif defined(NMBM_LOG_LEVEL_ERR)
#define NMBM_DEFAULT_LOG_LEVEL		3
#elif defined(NMBM_LOG_LEVEL_EMERG)
#define NMBM_DEFAULT_LOG_LEVEL		4
#elif defined(NMBM_LOG_LEVEL_NONE)
#define NMBM_DEFAULT_LOG_LEVEL		5
#else
#define NMBM_DEFAULT_LOG_LEVEL		1
#endif

#endif /* _NMBM_OS_H_ */
