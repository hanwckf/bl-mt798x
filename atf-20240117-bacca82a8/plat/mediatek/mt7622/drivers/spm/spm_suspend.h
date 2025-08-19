/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPM_SUSPEND_H
#define SPM_SUSPEND_H

/* cpu dormant return code */
#define CPU_DORMANT_RESET        0
#define CPU_DORMANT_ABORT        1

void spm_system_suspend(void);
void spm_system_suspend_finish(void);

#endif /* SPM_SUSPEND_H*/
