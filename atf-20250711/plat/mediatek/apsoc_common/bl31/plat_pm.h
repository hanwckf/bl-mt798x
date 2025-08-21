/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#ifndef _PLAT_PM_H_
#define _PLAT_PM_H_

#include <stdint.h>

/* Local power state for power domains in Run state. */
#define MTK_LOCAL_STATE_RUN		0
/* Local power state for retention. Valid only for CPU power domains */
#define MTK_LOCAL_STATE_RET		1
/* Local power state for OFF/power-down. Valid for CPU and cluster power
 * domains
 */
#define MTK_LOCAL_STATE_OFF		2

#define MTK_PWR_LVL0			0
#define MTK_PWR_LVL1			1
#define MTK_PWR_LVL2			2

/* Macros to read the MTK power domain state */
#define MTK_CORE_PWR_STATE(state)	(state)->pwr_domain_state[MTK_PWR_LVL0]
#define MTK_CLUSTER_PWR_STATE(state)	(state)->pwr_domain_state[MTK_PWR_LVL1]
#define MTK_SYSTEM_PWR_STATE(state)	((PLAT_MAX_PWR_LVL > MTK_PWR_LVL1) ? \
					 (state)->pwr_domain_state[MTK_PWR_LVL2] : 0)

extern uintptr_t secure_entrypoint;

void mt_platform_save_context(unsigned long mpidr);
void mt_platform_restore_context(unsigned long mpidr);

void mtk_system_pwr_domain_resume(void);

#endif /* _PLAT_PM_H_ */
