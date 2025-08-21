// SPDX-License-Identifier: GPL-2.0+

#ifndef _UN_XZ_H_
#define _UN_XZ_H_

#include <stddef.h>
#include <stdint.h>

#define UNXZ_OK			0
#define UNXZ_INIT_FAIL		1
#define UNXZ_FORMAT_FAIL	2
#define UNXZ_DEC_FAIL		3

/* Enable XZ_INTERNAL_CRC32 */
#ifdef XZ_INTERNAL_CRC32
#undef XZ_INTERNAL_CRC32
#define XZ_INTERNAL_CRC32	1
#endif

int unxz(const void *in_buf, size_t in_len, size_t *out_len,
	 const void *out_buf, size_t out_buf_size);

#endif /* _UN_XZ_H_ */
