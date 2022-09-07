/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <drivers/delay_timer.h>
#include <common/debug.h>
#include <mt7986_def.h>
#include <lib/mmio.h>
#include <gpt.h>

#define GPT4_BASE			0x80

#define US_PER_SECOND			1000000

static struct mtk_gpt gpt4 = {
	.soc = GPT_SOC_MT7986,
	.regbase = APXGPT_BASE + GPT4_BASE,
	.clkfreq = MTK_GPT_CLOCK_RATE
};

static timer_ops_t ops;

static uint32_t get_timer_value(void)
{
	uint32_t val = (uint32_t)mtk_gpt_get_counter(&gpt4);

	/*
	 * Generic delay timer implementation expects the timer to be a down
	 * counter. We apply bitwise NOT operator to the tick values returned
	 * by GPT to simulate the down counter. The value is 32 bits.
	 */
	return ~val;
}

static void mtk_timer_init_delay_timer(void)
{
	/* Value in ticks */
	unsigned int mult = US_PER_SECOND;

	/* Value in ticks per second (Hz) */
	unsigned int div  = MTK_GPT_CLOCK_RATE;

	/* Reduce multiplier and divider by dividing them repeatedly by 10 */
	while (((mult % 10U) == 0U) && ((div % 10U) == 0U)) {
		mult /= 10U;
		div /= 10U;
	}

	ops.get_timer_value	= get_timer_value;
	ops.clk_mult		= mult;
	ops.clk_div		= div;

	timer_init(&ops);

	VERBOSE("GPT timer configured with mult=%u and div=%u\n", mult, div);
}

void basic_timer_test(void)
{
	int i;
	uint64_t timeout;
	INFO("start gpt_timer test\n");
	for (i=0 ; i<10 ; i++) {
		INFO("0.%d\n", i);
		mdelay(100);
	}
	INFO("stop gpt_timer test\n");
	INFO("start arm_timer test\n");
	for (i=0 ; i<10 ; i++) {
		INFO("0.%d\n", i);
		timeout = timeout_init_us(100*1000);
		while (!timeout_elapsed(timeout)){;}
	}
	INFO("stop arm_timer test\n");
	INFO("compare gtp_timer and arm_timer\n");
	timeout = timeout_init_us(1000*1000);
	i = 0;
	while (!timeout_elapsed(timeout)) {
		INFO("0.%d\n", i++);
		mdelay(100);
	}
	INFO("all test done\n");

}

static void arm_timer_init(void)
{
	write_cntfrq_el0(ARM_TIMER_CLOCK_RATE);
}

void mtk_timer_init(void)
{
	mtk_gpt_init(&gpt4);
	mtk_timer_init_delay_timer();
	arm_timer_init();
}

uint64_t get_ticks(void)
{
	return mtk_gpt_get_counter(&gpt4);
}

uint64_t get_timer(uint64_t base)
{
	uint64_t tmc;

	tmc = mtk_gpt_get_timer(&gpt4);

	if (tmc >= base)
		return tmc - base;

	return ~(base - tmc);
}

uint64_t tick_to_time(uint64_t ticks)
{
	return mtk_gpt_counter_to_us(&gpt4, ticks);
}

uint64_t time_to_tick(uint64_t usec)
{
	return mtk_gpt_us_to_counter(&gpt4, usec);
}

void __udelay(uint64_t usec)
{
	mtk_gpt_udelay(&gpt4, usec);
}

void __mdelay(uint64_t msec)
{
	mtk_gpt_mdelay(&gpt4, msec);
}
