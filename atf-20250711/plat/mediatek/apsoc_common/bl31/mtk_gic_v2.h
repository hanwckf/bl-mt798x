/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2019, ARM Limited and Contributors. All rights reserved.
 */

#ifndef MTK_GIC_V2_H
#define MTK_GIC_V2_H

void plat_mt_gic_driver_init(void);
void plat_mt_gic_init(void);
void irq_raise_softirq(unsigned int map, unsigned int irq);
void mt_gic_dist_save(void);
void mt_gic_dist_restore(void);
void mt_gic_cpuif_setup(void);

#endif /* MTK_GIC_V2_H */
