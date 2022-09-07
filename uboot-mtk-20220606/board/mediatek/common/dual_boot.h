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

#ifdef CONFIG_MMC
struct mmc_boot_slot {
	const char *kernel;
	const char *rootfs;
};

struct mmc_boot_data {
	u32 dev;
	const struct mmc_boot_slot *boot_slots;
	const char *env_part;
	const char *rootfs_data_part;
	void *load_ptr;
};

int dual_boot_mmc(struct mmc_boot_data *mbd);
#endif /* CONFIG_MMC */

#endif /* _DUAL_BOOT_H_ */
