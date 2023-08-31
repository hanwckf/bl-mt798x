/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * A/B System implementation
 */

#ifndef _DUAL_BOOT_H_
#define _DUAL_BOOT_H_

#include <linux/types.h>

#define DUAL_BOOT_MAX_SLOTS			2

u32 dual_boot_get_current_slot(void);
u32 dual_boot_get_next_slot(void);
int dual_boot_set_current_slot(u32 slot);
bool dual_boot_is_slot_invalid(u32 slot);
int dual_boot_set_slot_invalid(u32 slot, bool invalid, bool save);

void bootargs_reset(void);
int bootargs_set(const char *key, const char *value);

struct dual_boot_slot {
	const char *kernel;
	const char *rootfs;
};

#endif /* _DUAL_BOOT_H_ */
