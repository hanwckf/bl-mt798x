/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image boot helper
 */

#ifndef _BOOT_HELPER_H_
#define _BOOT_HELPER_H_

#include <linux/types.h>

extern int board_boot_default(void);

int boot_from_mem(ulong data_load_addr);

struct bootarg {
	const char *key;
	const char *value;
};

int cmdline_merge(const char *cmdline, const struct bootarg *bootargs,
		  u32 count, char **result);

void list_all_args(const char *args);


const char *get_arg_next(const char *args, const char **param,
			 size_t *keylen);

#endif /* _BOOT_HELPER_H_ */
