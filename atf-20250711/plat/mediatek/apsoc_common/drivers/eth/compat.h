/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef _MTK_ETH_COMPAT_H_
#define _MTK_ETH_COMPAT_H_

#include <stdint.h>
#include <lib/mmio.h>
#include <drivers/delay_timer.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define __bf_shf(x) (__builtin_ffsll(x) - 1)

#define FIELD_PREP(_mask, _val)						\
	({								\
		((typeof(_mask))(_val) << __bf_shf(_mask)) & (_mask);	\
	})

#define FIELD_GET(_mask, _reg)						\
	({								\
		(typeof(_mask))(((_reg) & (_mask)) >> __bf_shf(_mask));	\
	})

#define roundup(x, y)							\
	({								\
		const typeof(y) __y = y;				\
		(((x) + (__y - 1)) / __y) * __y;			\
	})

#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

/**
 * wait_for_bit_x()	waits for bit set/cleared in register
 *
 * Function polls register waiting for specific bit(s) change
 * (either 0->1 or 1->0). It can fail under one conditions:
 * - Timeout
 * Function succeeds only if all bits of masked register are set/cleared
 * (depending on set option).
 *
 * @param reg		Register that will be read (using read_x())
 * @param mask		Bit(s) of register that must be active
 * @param set		Selects wait condition (bit set or clear)
 * @param timeout_ms	Timeout (in milliseconds)
 * Return:		0 on success, -ETIMEDOUT on failure
 */

#define BUILD_WAIT_FOR_BIT(sfx, type, read)				\
									\
static inline int wait_for_bit_##sfx(uintptr_t reg,			\
				     const type mask,			\
				     const bool set,			\
				     const unsigned int timeout_ms)	\
{									\
	type val;							\
	uint64_t tmo = timeout_init_us(timeout_ms * 1000);		\
									\
	while (1) {							\
		val = read(reg);					\
									\
		if (!set)						\
			val = ~val;					\
									\
		if ((val & mask) == mask)				\
			return 0;					\
									\
		if (timeout_elapsed(tmo))				\
			break;						\
									\
		udelay(1);						\
	}								\
									\
	return -ETIMEDOUT;						\
}

BUILD_WAIT_FOR_BIT(32, uint32_t, mmio_read_32)

#endif /* _MTK_ETH_COMPAT_H_ */
