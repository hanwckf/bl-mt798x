/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */
#ifndef __MTSPMC_H__
#define __MTSPMC_H__

/* #define FPGA_SMP		1 */
#define SPMC_SW_MODE		0
#define SPMC_DEBUG		1
#define SPMC_DVT		0
/* #define SPMC_DVT_UDELAY	0 */
#define SPMC_SPARK2		0
#define CONFIG_SPMC_MODE	1 /* 0:Legacy  1:HW  2:SW */

#define CPUTOP_MP0		0
#define CPUTOP_MP1		1
#define CPUTOP_MP2		2

#define STA_POWER_DOWN		0
#define STA_POWER_ON		1

#define MODE_SPMC_HW		0
#define MODE_AUTO_SHUT_OFF	1
#define MODE_DORMANT		2

int spmc_init(void);
int spmc_cputop_mpx_onoff(int cputop_mpx, int state, int mode);
int spmc_cpu_corex_onoff(int linear_id, int state, int mode);

void mcucfg_init_archstate(int cluster, int cpu, int arm64);
void mcucfg_set_bootaddr(int cluster, int cpu, uintptr_t bootaddr);
uintptr_t mcucfg_get_bootaddr(int cluster, int cpu);

void spark_enable(int cluster, int cpu);
void spark_disable(int cluster, int cpu);

#endif /* __MTCMOS_H__ */
