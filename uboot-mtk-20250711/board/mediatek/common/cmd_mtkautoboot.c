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
#include <menu.h>
#include <stdio.h>
#include <time.h>
#include <vsprintf.h>
#include <linux/types.h>
#include <linux/time.h>

#include "autoboot_helper.h"

#define AUTOBOOT_REPEAT_DELAY_SECONDS			5

static const struct bootmenu_entry *menu_entries;
static u32 menu_count;
static bool repeat_menu, in_menu;

void set_bootmenu_repeat(const char *fmt, ...)
{
	uint32_t remains, last_remains;
	uint64_t tmr, tmo;
	va_list args;

	if (!in_menu)
		return;

	if (fmt) {
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}

	printf("\nPress any key to return.\n\n");

	tmo = timer_get_us() + AUTOBOOT_REPEAT_DELAY_SECONDS * USEC_PER_SEC;
	last_remains = AUTOBOOT_REPEAT_DELAY_SECONDS + 1;

	do {
		tmr = timer_get_us();
		remains = (uint32_t)((tmo - tmr + USEC_PER_SEC - 1) / USEC_PER_SEC);

		if (remains < last_remains) {
			printf("\rReturning to bootmenu in %us ...", remains);
			last_remains = remains;
		}

		if (tstc()) {
			getchar();
			break;
		}
	} while (tmr < tmo);

	printf("\n");

	repeat_menu = true;
}

static int do_mtkautoboot(struct cmd_tbl *cmdtp, int flag, int argc,
			  char *const argv[])
{
	int i;
	char key[16];
	char val[256];
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

	in_menu = true;

	do {
		repeat_menu = false;
		menu_show(delay);
		delay = -1;
	} while (repeat_menu);

	in_menu = false;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkautoboot, 1, 0, do_mtkautoboot,
	   "Display MediaTek bootmenu", ""
);
