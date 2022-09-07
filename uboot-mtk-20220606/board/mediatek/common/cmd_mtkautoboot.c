// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic autoboot menu command
 */

#include <command.h>
#include <env.h>
#include <stdio.h>
#include <vsprintf.h>
#include <linux/types.h>

#include "autoboot_helper.h"

static const struct bootmenu_entry *menu_entries;
static u32 menu_count;

static int do_mtkautoboot(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	int i;
	char key[16];
	char val[256];
	char cmd[32];
	int delay = -1;
#ifdef CONFIG_MEDIATEK_BOOTMENU_COUNTDOWN
	const char *delay_str;
#endif

	board_bootmenu_entries(&menu_entries, &menu_count);

	if (!menu_entries || !menu_count) {
		printf("mtkautoboot is not configured!\n");
		return CMD_RET_FAILURE;
	}

	for (i = 0; i < menu_count; i++) {
		snprintf(key, sizeof(key), "bootmenu_%d", i);
		snprintf(val, sizeof(val), "%s=%s",
			 menu_entries[i].desc, menu_entries[i].cmd);
		env_set(key, val);
	}

	/*
	 * Remove possibly existed `next entry` to force bootmenu command to
	 * stop processing
	 */
	snprintf(key, sizeof(key), "bootmenu_%d", i);
	env_set(key, NULL);

#ifdef CONFIG_MEDIATEK_BOOTMENU_COUNTDOWN
	delay_str = env_get("bootmenu_delay");
	if (delay_str)
		delay = simple_strtol(delay_str, NULL, 10);
	else
		delay = CONFIG_MEDIATEK_BOOTMENU_DELAY;
#endif

	snprintf(cmd, sizeof(cmd), "bootmenu %d", delay);
	run_command(cmd, 0);

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkautoboot, 1, 0, do_mtkautoboot,
	   "Display MediaTek bootmenu", ""
);
