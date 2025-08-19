/*
 * Copyright (C) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include "gpt.h"

#define GPT_CON			0x00
#define CON_CLKDIV_S		10
#define CON_MODE_M		0x03
#define   CON_MODE_ONE_SHOT	0
#define   CON_MODE_REPEAT	1
#define   CON_MODE_KEEP_GO	2
#define   CON_MODE_FREE_RUN	3
#define CON_SW_CG		BIT(4)
#define CON_CLK			BIT(2)
#define CON_CLR			BIT(1)
#define CON_EN			BIT(0)

#define GPT_CLK			0x04
#define CLK_CLK			BIT(4)
#define CLK_CLKDIV_S		0

#define GPT_SEC			0x04

#define GPT_COUNT		0x08
#define GPT_COMPARE		0x0c
#define GPT_COUNT_H		0x10
#define GPT_COMPARE_H		0x14

#define CLKDIV_M		0x0f

#define US_PER_SECOND		1000000

struct mtk_gpt_soc_data {
	uint8_t clkmode_s;
	uint8_t clkcfg_reg;
	uint8_t clkdiv_s;
	uint8_t clksrc_bit;
};

static const struct mtk_gpt_soc_data gpt_soc_data[] = {
	[GPT_SOC_MT7622] = {
		.clkmode_s = 4,
		.clkcfg_reg = GPT_CLK,
		.clkdiv_s = CLK_CLKDIV_S,
		.clksrc_bit = 4,
	},
	[GPT_SOC_MT7981] = {
		.clkmode_s = 5,
		.clkcfg_reg = GPT_CON,
		.clkdiv_s = CON_CLKDIV_S,
		.clksrc_bit = 2,
	},
	[GPT_SOC_MT7986] = {
		.clkmode_s = 5,
		.clkcfg_reg = GPT_CON,
		.clkdiv_s = CON_CLKDIV_S,
		.clksrc_bit = 2,
	}
};

void mtk_gpt_stop(struct mtk_gpt *gpt)
{
	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	mmio_clrbits_32(gpt->regbase + GPT_CON, CON_EN);
}

void mtk_gpt_start(struct mtk_gpt *gpt)
{
	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	mmio_write_32(gpt->regbase + GPT_CON, 0);
	mmio_clrbits_32(gpt->regbase + gpt_soc_data[gpt->soc].clkcfg_reg,
			CLKDIV_M | BIT(gpt_soc_data[gpt->soc].clksrc_bit));
	mmio_setbits_32(gpt->regbase + GPT_CON, CON_EN |
			CON_MODE_FREE_RUN << gpt_soc_data[gpt->soc].clkmode_s);
}

void mtk_gpt_reset(struct mtk_gpt *gpt)
{
	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	mmio_setbits_32(gpt->regbase + GPT_CON, CON_CLR);

	gpt->cnt_l = gpt->cnt_h = 0;
}

void mtk_gpt_init(struct mtk_gpt *gpt)
{
	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	gpt->hz = US_PER_SECOND;
	gpt->freq = gpt->clkfreq;

	/* Reduce multiplier and divider by dividing them repeatedly by 10 */
	while (((gpt->hz % 10U) == 0U) && ((gpt->freq % 10U) == 0U)) {
		gpt->hz /= 10U;
		gpt->freq /= 10U;
	}

	mtk_gpt_stop(gpt);
	mtk_gpt_reset(gpt);
	mtk_gpt_start(gpt);
}

uint64_t mtk_gpt_get_counter(struct mtk_gpt *gpt)
{
	uint32_t hi, lo;

	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	if (gpt->cnt64) {
		do {
			hi = mmio_read_32(gpt->regbase + GPT_COUNT_H);
			gpt->cnt_l = mmio_read_32(gpt->regbase + GPT_COUNT);
			gpt->cnt_h = mmio_read_32(gpt->regbase + GPT_COUNT_H);
		} while (gpt->cnt_h != hi);
	} else {
		lo = mmio_read_32(gpt->regbase + GPT_COUNT);

		if (lo < gpt->cnt_l)
			gpt->cnt_h++;
		gpt->cnt_l = lo;
	}

	return (((uint64_t)gpt->cnt_h) << 32) | gpt->cnt_l;
}

uint64_t mtk_gpt_counter_to_us(struct mtk_gpt *gpt, uint64_t cnt)
{
	uint64_t n;

	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	if ((ULLONG_MAX - gpt->freq + 1) / gpt->hz <= cnt)
		n = ULLONG_MAX - gpt->freq + 1;
	else
		n = cnt * gpt->hz;

	return div_round_up(n, gpt->freq);
}

uint64_t mtk_gpt_us_to_counter(struct mtk_gpt *gpt, uint64_t usec)
{
	uint64_t n;

	assert(gpt && gpt->soc < __GPT_SOC_MAX);

	if ((ULLONG_MAX - gpt->hz + 1) / gpt->freq <= usec)
		n = ULLONG_MAX - gpt->hz + 1;
	else
		n = usec * gpt->freq;

	return div_round_up(n, gpt->hz);
}

void mtk_gpt_udelay(struct mtk_gpt *gpt, uint64_t usec)
{
	uint64_t st, delta;

	st = mtk_gpt_get_counter(gpt);
	delta = mtk_gpt_us_to_counter(gpt, usec) + st + 1;

	while (mtk_gpt_get_counter(gpt) < delta)
		;
}
