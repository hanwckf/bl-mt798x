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

#define CPUXGPT_REG_CONTROL		0x00
#define CPUXGPT_CONTROL_DIV_S		8
#define CPUXGPT_CONTROL_DIV_M		0x07

#define CPUXGPT_REG_COUNTER_SET_L	0x08
#define CPUXGPT_REG_COUNTER_SET_H	0x0c

#define ARCH_TIMER_FREQ_DIV		2

#define ARCH_TIMER_HZ			1000000

static void write_cpuxgpt(uint32_t reg_index, uint32_t value)
{
	mmio_write_32((uintptr_t)&mt7622_mcucfg->xgpt_idx, reg_index);
	mmio_write_32((uintptr_t)&mt7622_mcucfg->xgpt_ctl, value);
}

static void cpuxgpt_set_init_cnt(uint32_t cnt_h, uint32_t cnt_l)
{
	write_cpuxgpt(CPUXGPT_REG_COUNTER_SET_H, cnt_h);
	/* update count when countL programmed */
	write_cpuxgpt(CPUXGPT_REG_COUNTER_SET_L, cnt_l);
}

void generic_timer_backup(void)
{
	uint64_t cval;

	cval = read_cntpct_el0();
	cpuxgpt_set_init_cnt((uint32_t)(cval >> 32),
			     (uint32_t)(cval & 0xffffffff));
}

void plat_mt_cpuxgpt_init(void)
{
	uint32_t xtal_clk;

	/* Set clock divsior to 2 for kernel arch timer */
	write_cpuxgpt(CPUXGPT_REG_CONTROL,
		      ARCH_TIMER_FREQ_DIV << CPUXGPT_CONTROL_DIV_S);

	/* Set CPUXGPT counter to 0 */
	cpuxgpt_set_init_cnt(0, 0);

	/* Record timer frequency */
	if (mmio_read_32(RGU_STRAP_PAR) & RGU_INPUT_TYPE)
		xtal_clk = 40000000;
	else
		xtal_clk = 25000000;

	write_cntfrq_el0(xtal_clk / ARCH_TIMER_FREQ_DIV);
}

uint64_t tick_to_time(uint64_t ticks)
{
	uint32_t freq = read_cntfrq_el0();
	uint64_t n;

	if ((ULLONG_MAX - freq + 1) / ARCH_TIMER_HZ <= ticks)
		n = ULLONG_MAX - freq + 1;
	else
		n = ticks * ARCH_TIMER_HZ;

	return div_round_up(n, freq);
}

uint64_t time_to_tick(uint64_t usec)
{
	uint32_t freq = read_cntfrq_el0();
	uint64_t n;

	if ((ULLONG_MAX - ARCH_TIMER_HZ + 1) / freq <= usec)
		n = ULLONG_MAX - ARCH_TIMER_HZ + 1;
	else
		n = usec * freq;

	return div_round_up(n, ARCH_TIMER_HZ);
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
