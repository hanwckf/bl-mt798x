/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RNG_H
#define RNG_H

#include <platform_def.h>

#define MTK_RETRY_CNT        10
#define MTK_TIMEOUT_POLL     1000

#define RNG_DEFAULT_CUTOFF   0x0f003f30

/*******************************************************************************
 * TRNG related constants
 ******************************************************************************/
#define RNG_STATUS          (TRNG_BASE + 0x0004)
#define RNG_SWRST           (TRNG_BASE + 0x0010)
#define RNG_IRQ_CFG         (TRNG_BASE + 0x0014)
#define RNG_EN              (TRNG_BASE + 0x0020)
#define RNG_HTEST           (TRNG_BASE + 0x0028)
#define RNG_OUT             (TRNG_BASE + 0x0030)
#define RNG_RAW             (TRNG_BASE + 0x0038)
#define RNG_SRC             (TRNG_BASE + 0x0050)
#define RNG_ERROR           GENMASK_32(28, 24)
#define RAW_VALID           BIT(12)
#define DRBG_VALID          BIT(4)
#define RAW_EN              BIT(8)
#define NRBG_EN             BIT(4)
#define DRBG_EN             BIT(0)
#define IRQ_EN              BIT(0)
#define SWRST_B             BIT(0)
#define TRNG_SWRST_SET_REG  (INFRACFG_AO_BASE +	0x150)
#define TRNG_SWRST_CLR_REG  (INFRACFG_AO_BASE +	0x154)
#define RNG_SWRST_B         BIT(13)


uint32_t plat_get_rnd(uint32_t *val);

#endif
