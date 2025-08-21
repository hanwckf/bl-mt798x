/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
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
#define RNG_CTRL            (TRNG_BASE + 0x0000)
#define RNG_TIME            (TRNG_BASE + 0x0004)
#define RNG_DATA            (TRNG_BASE + 0x0008)
#define RNG_CONF            (TRNG_BASE + 0x000C)
#define RNG_INT_SET         (TRNG_BASE + 0x0010)
#define RNG_INT_CLR         (TRNG_BASE + 0x0014)
#define RNG_INT_MSK         (TRNG_BASE + 0x0018)

#define RNG_EN              BIT(0)
#define RNG_READY           BIT(31)

uint32_t plat_get_rnd(uint32_t *val);

#endif
