/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic autoboot menu helper
 */

#ifndef _AUTOBOOT_HELPER_H_
#define _AUTOBOOT_HELPER_H_

#include <linux/types.h>
#include <stdbool.h>

struct bootmenu_entry {
	const char *desc;
	const char *cmd;
};

extern void board_bootmenu_entries(const struct bootmenu_entry **menu,
				   u32 *count);

#endif /* _AUTOBOOT_HELPER_H_ */
