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

#define BOOT_PARAM_STR_MAX_LEN			256

extern int board_boot_default(bool do_boot);

int boot_from_mem(ulong data_load_addr);

struct arg_pair {
	const char *key;
	const char *value;
};

struct arg_list {
	const char *name;
	struct arg_pair *ap;
	u32 max;
	u32 used;
};

void bootargs_reset(void);
int bootargs_set(const char *key, const char *value);
void bootargs_unset(const char *key);

void fdtargs_reset(void);
int fdtargs_set(const char *prop, const char *value);
void fdtargs_unset(const char *prop);

void list_all_args(const char *args);


const char *get_arg_next(const char *args, const char **param,
			 size_t *keylen);

#endif /* _BOOT_HELPER_H_ */
