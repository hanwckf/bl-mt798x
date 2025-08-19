/*
 * Copyright (C) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MTK_GPT_H_
#define _MTK_GPT_H_

#include <limits.h>
#include <stdint.h>
#include <stdbool.h>

enum mtk_gpt_soc {
	GPT_SOC_MT7622,
	GPT_SOC_MT7981,
	GPT_SOC_MT7986,

	__GPT_SOC_MAX
};

struct mtk_gpt {
	enum mtk_gpt_soc soc;
	bool cnt64;
	uintptr_t regbase;
	uint32_t clkfreq;

	/* private fields for gpt driver */
	uint32_t hz;
	uint32_t freq;
	uint32_t cnt_l;
	uint32_t cnt_h;
};

void mtk_gpt_stop(struct mtk_gpt *gpt);
void mtk_gpt_start(struct mtk_gpt *gpt);
void mtk_gpt_reset(struct mtk_gpt *gpt);
void mtk_gpt_init(struct mtk_gpt *gpt);
uint64_t mtk_gpt_get_counter(struct mtk_gpt *gpt);
uint64_t mtk_gpt_counter_to_us(struct mtk_gpt *gpt, uint64_t cnt);
uint64_t mtk_gpt_us_to_counter(struct mtk_gpt *gpt, uint64_t usec);
void mtk_gpt_udelay(struct mtk_gpt *gpt, uint64_t usec);

static inline uint64_t mtk_gpt_get_timer(struct mtk_gpt *gpt)
{
	return mtk_gpt_counter_to_us(gpt, mtk_gpt_get_counter(gpt));
}

static inline void mtk_gpt_mdelay(struct mtk_gpt *gpt, uint64_t msec)
{
	uint64_t usec;

	if (ULLONG_MAX / 1000 <= msec)
		usec = ULLONG_MAX;
	else
		usec = msec * 1000;

	mtk_gpt_udelay(gpt, usec);
}

#endif
