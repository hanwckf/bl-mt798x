// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <stdint.h>
#include <stdbool.h>
#include <arch_helpers.h>

static inline uint64_t timeout_cnt_us2cnt(uint32_t us)
{
	return ((uint64_t)us * (uint64_t)read_cntfrq_el0()) / 1000000ULL;
}

static inline uint64_t timeout_init_us(uint32_t us)
{
	uint64_t cnt = timeout_cnt_us2cnt(us);

	cnt += read_cntpct_el0();

	return cnt;
}

static inline bool timeout_elapsed(uint64_t expire_cnt)
{
	return read_cntpct_el0() > expire_cnt;
}

void udelay(uint32_t usec)
{
	uint64_t tmo = timeout_init_us(usec);

	while (!timeout_elapsed(tmo))
		;
}
