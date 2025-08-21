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

#define PART_FIRMWARE_NAME	"firmware"
#define PART_KERNEL_NAME	"kernel"
#define PART_ROOTFS_NAME	"rootfs"
#define PART_ROOTFS_DATA_NAME	"rootfs_data"

void dual_boot_disable(void);
u32 dual_boot_get_current_slot(void);
u32 dual_boot_get_next_slot(void);
int dual_boot_set_current_slot(u32 slot);
bool dual_boot_is_slot_invalid(u32 slot);
int dual_boot_set_slot_invalid(u32 slot, bool invalid, bool save);

bool dual_boot_get_boot_count(u32 *retslot, u32 *retcnt);
void dual_boot_set_boot_count(u32 slot, u32 count);

int dual_boot_set_defaults(void *fdt);

struct dual_boot_slot {
	const char *kernel;
	const char *rootfs;
	const char *rootfs_data;
};

extern const struct dual_boot_slot dual_boot_slots[DUAL_BOOT_MAX_SLOTS];

struct dual_boot_priv {
	int (*boot_slot)(struct dual_boot_priv *priv, u32 slot, bool do_boot);
};

int dual_boot(struct dual_boot_priv *priv, bool do_boot);

#endif /* _DUAL_BOOT_H_ */
