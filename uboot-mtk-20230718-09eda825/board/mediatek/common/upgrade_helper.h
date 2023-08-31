/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data upgrading helper
 */

#ifndef _UPGRADE_HELPER_H_
#define _UPGRADE_HELPER_H_

#include <linux/types.h>
#include <stdbool.h>

enum data_post_upgrade_action {
	UPGRADE_ACTION_NONE,
	UPGRADE_ACTION_REBOOT,
	UPGRADE_ACTION_BOOT,
	UPGRADE_ACTION_CUSTOM,
};

struct data_part_entry;

typedef int (*data_handler_t)(void *priv, const struct data_part_entry *dpe,
			      const void *data, size_t size);

struct data_part_entry {
	const char *name;
	const char *abbr;
	const char *env_name;
	enum data_post_upgrade_action post_action;
	const char *custom_action_prompt;
	void *priv;
	data_handler_t validate;
	data_handler_t write;
	data_handler_t do_post_action;
};

extern void board_upgrade_data_parts(const struct data_part_entry **dpes,
				     u32 *count);

int check_data_size(u64 total_size, u64 offset, size_t max_size, size_t size,
		    bool write);
bool verify_data(const u8 *orig, const u8 *rdback, u64 offset, size_t size);

#endif /* _UPGRADE_HELPER_H_ */
