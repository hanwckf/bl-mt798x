/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <cpuxgpt.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <drivers/generic_delay_timer.h>

#define CPUXGPT_REG_CONTROL	  0x00
#define CPUXGPT_REG_COUNTER_SET_L 0x08
#define CPUXGPT_REG_COUNTER_SET_H 0x0c
#define ARCH_TIMER_HZ		  1000000

/* This will differ when in FPGA or ASIC mode */
#define SYSTIMER_BASE 0x10017000

static bool clock_rate_update_flag;

unsigned int plat_get_syscnt_freq2(void)
{
	if (!clock_rate_update_flag)
		return ARM_TIMER_CLOCK_RATE_0;
	else
		return ARM_TIMER_CLOCK_RATE_1;
}

static void write_cpuxgpt(uint32_t reg_index, uint32_t value)
{
	mmio_write_32(SYSTIMER_BASE + reg_index, value);
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
	/* Make system timer output clock rate follow the input clock rate
         * by disable BIT[12] COMP_25_EN and BIT[11] COMP_20_EN,
         * In FPGA mode, the output clock will be passed to the ARM ARCH timer
         * directlly. In ASIC mode, the output clock will be devided by 2 and
	 * passed to ARM ARCH TIMER
         */
	write_cpuxgpt(CPUXGPT_REG_CONTROL, 0x15);
	/* Record timer frequency */
	if (!clock_rate_update_flag)
		write_cntfrq_el0(ARM_TIMER_CLOCK_RATE_0);
	else
		write_cntfrq_el0(ARM_TIMER_CLOCK_RATE_1);
}

void plat_mt_cpuxgpt_pll_update_sync(void)
{
	if (!clock_rate_update_flag) {
		clock_rate_update_flag = true;
		write_cntfrq_el0(ARM_TIMER_CLOCK_RATE_1);
	}

	/* update timer frequency by initial it again */
	generic_delay_timer_init();
}
