/* SPDX-License-Identifier: GPL 2.0+ OR BSD-3-Clause */

#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <stdint.h>
#include <arm_acle.h>

#define roundup(x, y) ({				\
	const typeof(y) __y = y;			\
	(((x) + (__y - 1)) / __y) * __y;		\
})

#define BITS_PER_LONG		(8 * sizeof(long))
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)

static inline int test_bit(int nr, const void *addr)
{
	return ((unsigned char *)addr)[nr >> 3] & (1U << (nr & 7));
}

static inline int test_and_set_bit(int nr, volatile void *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;

	*p = old | mask;
	return (old & mask) != 0;
}

static inline void generic_set_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p  |= mask;
}

static inline void generic_clear_bit(int nr, volatile unsigned long *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);

	*p &= ~mask;
}

static inline int test_and_clear_bit(int nr, volatile void *addr)
{
	unsigned long mask = BIT_MASK(nr);
	unsigned long *p = ((unsigned long *)addr) + BIT_WORD(nr);
	unsigned long old = *p;

	*p = old & ~mask;
	return (old & mask) != 0;
}

#if (ARM_ARCH_MAJOR > 7)
static inline uint32_t ubi_crc32(uint32_t crc, const void *buf, size_t size)
{
	const uint8_t *local_buf = buf;
	size_t local_size = size;

	/*
	 * calculate CRC over byte data
	 */
	while (local_size != 0UL) {
		crc = __crc32b(crc, *local_buf);
		local_buf++;
		local_size--;
	}

	return crc;
}
#else
uint32_t ubi_crc32(uint32_t crc, const void *buf, size_t size);
#endif

#endif /* _COMPAT_H_ */
