
// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <lib/mmio.h>
#include <lib/utils_def.h>
#include <mt7986_def.h>
#include <limits.h>

#define GPT4_CLOCK		20 /* 20000000 */
#define US_PER_SECOND		1 /* 1000000 */

#define GPT4_BASE		0x80

#define GPT_CON			0x00
#define CON_MODE_S		5
#define CON_MODE_M		0x03
#define   CON_MODE_FREE_RUN	3
#define CLKDIV_M		0x0f
#define CON_CLK			BIT(2)
#define CON_CLR			BIT(1)
#define CON_EN			BIT(0)

#define GPT_COUNT		0x08

static uint32_t cnt_l, cnt_h;

void mtk_bl2pl_timer_init(void)
{
	mmio_write_32(APXGPT_BASE + GPT4_BASE + GPT_CON,
		      CON_EN | (CON_MODE_FREE_RUN << CON_MODE_S) | CON_CLR);

	cnt_l = cnt_h = 0;
}

static uint64_t mtk_bl2pl_timer_get_counter(void)
{
	uint32_t cnt = mmio_read_32(APXGPT_BASE + GPT4_BASE + GPT_COUNT);

	if (cnt < cnt_l)
		cnt_h++;
	cnt_l = cnt;

	return (((uint64_t)cnt_h) << 32) | cnt_l;
}

static uint64_t mtk_bl2pl_timer_us_to_counter(uint64_t usec)
{
	uint64_t n;

	if ((ULLONG_MAX - US_PER_SECOND + 1) / GPT4_CLOCK <= usec)
		n = ULLONG_MAX - US_PER_SECOND + 1;
	else
		n = usec * GPT4_CLOCK;

	return div_round_up(n, US_PER_SECOND);
}

void udelay(uint32_t usec)
{
	uint64_t st, delta;

	st = mtk_bl2pl_timer_get_counter();
	delta = mtk_bl2pl_timer_us_to_counter(usec) + st + 1;

	while (mtk_bl2pl_timer_get_counter() < delta)
		;
}
