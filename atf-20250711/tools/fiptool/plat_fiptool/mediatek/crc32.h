/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Author: Weijie Gao <hackpascal@gmail.com>
 *
 * CRC32 checksum helpers
 */
#pragma once

#ifndef _CRC32_H_
#define _CRC32_H_

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_no_comp(uint32_t crc, const void *data, size_t length);

static inline uint32_t crc32(uint32_t crc, const void *data, size_t length)
{
	/* reflected */
	return crc32_no_comp(crc ^ 0xffffffff, data, length) ^ 0xffffffff;
}

#endif /* _CRC32_H_ */
