/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 */

#ifndef MTK_GIC_V3_H
#define MTK_GIC_V3_H

#include <platform_def.h>

#define GIC_INT_MASK			(MCUCFG_BASE + 0x5e8)
#define GIC500_ACTIVE_SEL_SHIFT		3
#define GIC500_ACTIVE_SEL_MASK		(0x7 << GIC500_ACTIVE_SEL_SHIFT)
#define GIC500_ACTIVE_CPU_SHIFT		16
#define GIC500_ACTIVE_CPU_MASK		(0xff << GIC500_ACTIVE_CPU_SHIFT)

void plat_mt_gic_init(void);
void mt_gic_cpuif_enable(void);
void mt_gic_cpuif_disable(void);
void mt_gic_rdistif_init(void);
void mt_gic_distif_save(void);
void mt_gic_distif_restore(void);
void mt_gic_rdistif_save(void);
void mt_gic_rdistif_restore(void);

#endif /* MTK_GIC_V3_H */
