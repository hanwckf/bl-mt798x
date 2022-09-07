/*
 * Copyright (c) 2015, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <cpuxgpt.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <mcucfg.h>
#include <limits.h>

void generic_timer_backup(void)
{

}

void plat_mt_cpuxgpt_init(void)
{
	write_cntfrq_el0(ARCH_TIMER_CLK);
}

uint64_t tick_to_time(uint64_t ticks)
{
	uint32_t freq = read_cntfrq_el0();
	uint64_t n;

	if ((ULLONG_MAX - freq + 1) / ARCH_TIMER_CLK <= ticks)
		n = ULLONG_MAX - freq + 1;
	else
		n = ticks * ARCH_TIMER_CLK;

	return div_round_up(n, freq);
}

uint64_t time_to_tick(uint64_t usec)
{
	uint32_t freq = read_cntfrq_el0();
	uint64_t n;

	if ((ULLONG_MAX - ARCH_TIMER_CLK + 1) / freq <= usec)
		n = ULLONG_MAX - ARCH_TIMER_CLK + 1;
	else
		n = usec * freq;

	return div_round_up(n, ARCH_TIMER_CLK);
}

uint64_t get_ticks(void)
{
	return read_cntpct_el0();
}

uint64_t get_timer(uint64_t base)
{
	uint64_t tmc;

	tmc = tick_to_time(read_cntpct_el0());

	if (tmc >= base)
		return tmc - base;

	return ~(base - tmc);
}
