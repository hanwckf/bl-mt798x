/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_SNAND_OS_H_
#define _MTK_SNAND_OS_H_

#include <common.h>
#include <cpu_func.h>
#include <errno.h>
#include <div64.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdarg.h>
#include <linux/types.h>
#include <asm/io.h>
#include <linux/bitops.h>
#include <linux/sizes.h>
#include <linux/iopoll.h>

#ifndef ARCH_DMA_MINALIGN
#define ARCH_DMA_MINALIGN		64
#endif

struct mtk_snand_plat_dev {
	ulong unused;
};

/* Polling helpers */
#define read16_poll_timeout(addr, val, cond, sleep_us, timeout_us) \
	readw_poll_timeout((addr), (val), (cond), (timeout_us))

#define read32_poll_timeout(addr, val, cond, sleep_us, timeout_us) \
	readl_poll_timeout((addr), (val), (cond), (timeout_us))

/* Timer helpers */
typedef uint64_t mtk_snand_time_t;

static inline mtk_snand_time_t timer_get_ticks(void)
{
	return get_ticks();
}

static inline mtk_snand_time_t timer_time_to_tick(uint32_t timeout_us)
{
	return usec_to_tick(timeout_us);
}

static inline bool timer_is_timeout(mtk_snand_time_t start_tick,
				    mtk_snand_time_t timeout_tick)
{
	return get_ticks() - start_tick > timeout_tick;
}

/* Memory helpers */
static inline void *generic_mem_alloc(struct mtk_snand_plat_dev *pdev,
				      size_t size)
{
	return calloc(1, size);
}

static inline void generic_mem_free(struct mtk_snand_plat_dev *pdev, void *ptr)
{
	free(ptr);
}

static inline void *dma_mem_alloc(struct mtk_snand_plat_dev *pdev, size_t size)
{
	return memalign(ARCH_DMA_MINALIGN, size);
}

static inline void dma_mem_free(struct mtk_snand_plat_dev *pdev, void *ptr)
{
	free(ptr);
}

static inline int dma_mem_map(struct mtk_snand_plat_dev *pdev, void *vaddr,
			      uintptr_t *dma_addr, size_t size, bool to_device)
{
	size_t cachelen = roundup(size, ARCH_DMA_MINALIGN);
	uintptr_t endaddr = (uintptr_t)vaddr + cachelen;

	if (to_device)
		flush_dcache_range((uintptr_t)vaddr, endaddr);
	else
		invalidate_dcache_range((uintptr_t)vaddr, endaddr);

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

#endif /* _MTK_SNAND_OS_H_ */
