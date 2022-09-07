/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_SNAND_OS_H_
#define _MTK_SNAND_OS_H_

#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <lib/mmio.h>
#include <lib/utils_def.h>

#include <timer.h>
#include <mempool.h>

struct mtk_snand_plat_dev {
	unsigned long unused;
};

#define ENOTSUPP			524

#define __iomem

#define DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))
#define unlikely(x)			(x)

#define BITS_PER_BYTE			8

/* Size definitions */
#define SZ_512				0x00000200
#define SZ_2K				0x00000800
#define SZ_4K				0x00001000
#define SZ_8K				0x00002000
#define SZ_16K				0x00004000

/* Register r/w helpers */
static inline uint32_t readl(void __iomem *reg)
{
	return mmio_read_32((uintptr_t)reg);
}

static inline void writel(uint32_t val, void __iomem *reg)
{
	mmio_write_32((uintptr_t)reg, val);
}

static inline uint16_t readw(void __iomem *reg)
{
	return mmio_read_16((uintptr_t)reg);
}

static inline void writew(uint16_t val, void __iomem *reg)
{
	mmio_write_16((uintptr_t)reg, val);
}

/* Polling helpers */
#define readx_poll_timeout(op, addr, val, cond, timeout_us)	\
({ \
	uint64_t start_us = get_timer(0); \
	for (;;) { \
		(val) = op(addr); \
		if (cond) \
			break; \
		if (timeout_us && get_timer(start_us) > timeout_us) { \
			(val) = op(addr); \
			break; \
		} \
	} \
	(cond) ? 0 : -ETIMEDOUT; \
})

#define read16_poll_timeout(addr, val, cond, sleep_us, timeout_us) \
	readx_poll_timeout(readw, (addr), (val), (cond), (timeout_us))

#define read32_poll_timeout(addr, val, cond, sleep_us, timeout_us) \
	readx_poll_timeout(readl, (addr), (val), (cond), (timeout_us))


/* Timer helpers */
typedef uint64_t mtk_snand_time_t;

static inline mtk_snand_time_t timer_get_ticks(void)
{
	return get_ticks();
}

static inline mtk_snand_time_t timer_time_to_tick(uint32_t timeout_us)
{
	return time_to_tick(timeout_us);
}

static inline bool timer_is_timeout(mtk_snand_time_t start_tick,
				    mtk_snand_time_t timeout_tick)
{
	return get_ticks() > start_tick + timeout_tick;
}

/* Memory helpers */
static inline void *generic_mem_alloc(struct mtk_snand_plat_dev *pdev,
				      size_t size)
{
	return mtk_mem_pool_alloc(size);
}

static inline void generic_mem_free(struct mtk_snand_plat_dev *pdev, void *ptr)
{
}

static inline void *dma_mem_alloc(struct mtk_snand_plat_dev *pdev, size_t size)
{
	return mtk_mem_pool_alloc(size);
}

static inline void dma_mem_free(struct mtk_snand_plat_dev *pdev, void *ptr)
{
}

static inline int dma_mem_map(struct mtk_snand_plat_dev *pdev, void *vaddr,
			      uintptr_t *dma_addr, size_t size, bool to_device)
{
	*dma_addr = (uintptr_t)vaddr;

	return 0;
}

static inline void dma_mem_unmap(struct mtk_snand_plat_dev *pdev,
				 uintptr_t dma_addr, size_t size,
				 bool to_device)
{
}

/* Interrupt helpers */
static inline void irq_completion_done(struct mtk_snand_plat_dev *pdev)
{
}

static inline void irq_completion_init(struct mtk_snand_plat_dev *pdev)
{
}

static inline int irq_completion_wait(struct mtk_snand_plat_dev *pdev,
				      void __iomem *reg, uint32_t bit,
				      uint32_t timeout_us)
{
	uint32_t val;

	return read32_poll_timeout(reg, val, val & bit, 0, timeout_us);
}

/* Bitwise functions */
static inline int ffs(int x)
{
	int r = 1;

	if (!x)
		return 0;

	if (!(x & 0xffff)) {
		x >>= 16;
		r += 16;
	}

	if (!(x & 0xff)) {
		x >>= 8;
		r += 8;
	}

	if (!(x & 0xf)) {
		x >>= 4;
		r += 4;
	}

	if (!(x & 3)) {
		x >>= 2;
		r += 2;
	}

	if (!(x & 1)) {
		x >>= 1;
		r += 1;
	}

	return r;
}

static inline int fls(int x)
{
	int r = 32;

	if (!x)
		return 0;

	if (!(x & 0xffff0000u)) {
		x <<= 16;
		r -= 16;
	}

	if (!(x & 0xff000000u)) {
		x <<= 8;
		r -= 8;
	}

	if (!(x & 0xf0000000u)) {
		x <<= 4;
		r -= 4;
	}

	if (!(x & 0xc0000000u)) {
		x <<= 2;
		r -= 2;
	}

	if (!(x & 0x80000000u)) {
		x <<= 1;
		r -= 1;
	}

	return r;
}

static inline unsigned int hweight8(unsigned int w)
{
	unsigned int res = (w & 0x55) + ((w >> 1) & 0x55);
	res = (res & 0x33) + ((res >> 2) & 0x33);
	return (res & 0x0f) + ((res >> 4) & 0x0f);
}

static inline unsigned int hweight32(unsigned int w)
{
	unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
	res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
	res = (res & 0x0f0f0f0f) + ((res >> 4) & 0x0f0f0f0f);
	res = (res & 0x00ff00ff) + ((res >> 8) & 0x00ff00ff);
	return (res & 0x0000ffff) + ((res >> 16) & 0x0000ffff);
}

#endif /* _MTK_SNAND_OS_H_ */
