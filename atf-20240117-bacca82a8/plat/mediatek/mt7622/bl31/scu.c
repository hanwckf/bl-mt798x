/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <mcucfg.h>

void disable_scu(unsigned long mpidr)
{
	/* disable CA7L snoop function */
	mmio_setbits_32((uintptr_t)&mt7622_mcucfg->mp0_axi_config,
			MP0_ACINACTM);
}

void enable_scu(unsigned long mpidr)
{
	mmio_clrbits_32((uintptr_t)&mt7622_mcucfg->mp0_axi_config,
			MP0_ACINACTM);
}
