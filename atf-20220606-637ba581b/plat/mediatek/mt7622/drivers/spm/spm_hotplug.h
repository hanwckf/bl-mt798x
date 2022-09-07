/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPM_HOTPLUG_H
#define SPM_HOTPLUG_H

void spm_clear_hotplug(void);
void spm_hotplug_off(unsigned long mpidr);
void spm_hotplug_on(unsigned long mpidr);

#endif /* SPM_HOTPLUG_H */
