/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <errno.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <lib/spinlock.h>
#include <common/debug.h>
#include "rng.h"

static spinlock_t rng_lock;

static void plat_trng_external_swrst(void)
{
	/* reset whole rng module */
	mmio_setbits_32(TRNG_SWRST_SET_REG, RNG_SWRST_B);
	mmio_setbits_32(TRNG_SWRST_CLR_REG, RNG_SWRST_B);

	/* disable irq */
	mmio_clrbits_32(RNG_IRQ_CFG, IRQ_EN);

	/* set default cutoff value */
	mmio_write_32(RNG_HTEST, RNG_DEFAULT_CUTOFF);

	/* enable rng */
	mmio_setbits_32(RNG_EN, DRBG_EN | NRBG_EN);
}

static uint32_t plat_trng_prng(uint32_t *rand)
{
	uint64_t time = timeout_init_us(MTK_TIMEOUT_POLL);
	uint32_t retry_times = 0;

	if (rand == NULL)
		return -EINVAL;

	/* get random number form hwrng */
	mmio_clrbits_32(RNG_IRQ_CFG, IRQ_EN);

	mmio_write_32(RNG_HTEST, RNG_DEFAULT_CUTOFF);
	/* enable rng module */
	mmio_setbits_32(RNG_EN, (DRBG_EN | NRBG_EN));

	while (!(mmio_read_32(RNG_STATUS) & DRBG_VALID)) {
		if (mmio_read_32(RNG_STATUS) & RNG_ERROR) {
			mmio_clrbits_32(RNG_EN, (DRBG_EN | NRBG_EN));

			mmio_clrbits_32(RNG_SWRST, SWRST_B);
			mmio_setbits_32(RNG_SWRST, SWRST_B);

			mmio_setbits_32(RNG_EN, (DRBG_EN | NRBG_EN));
		}

		if (timeout_elapsed(time)) {
			plat_trng_external_swrst();
			time = timeout_init_us(MTK_TIMEOUT_POLL);
			retry_times++;
		}

		if (retry_times > MTK_RETRY_CNT) {
			ERROR("%s: trng NOT ready\n", __func__);
			return -EAGAIN;
		}
	}

	*rand = mmio_read_32(RNG_OUT);

	return 0;
}

/*
 * plat_get_rnd - get 32-bit random number data which is used from kernel
 * output - val: output buffer contains 32-bit rnd
 */
uint32_t plat_get_rnd(uint32_t *val)
{
	uint32_t ret;

	if (val == NULL)
		return -EINVAL;

	spin_lock(&rng_lock);
	ret = plat_trng_prng(val);
	spin_unlock(&rng_lock);

	return ret;
}
