// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data loading command
 */

#include <command.h>
#include <env.h>
#include <image.h>
#include <vsprintf.h>
#include <linux/stringify.h>
#include <linux/types.h>

#include "load_data.h"
#include "colored_print.h"

static int run_image(ulong data_addr, size_t data_size)
{
	char *argv[2], str[64];

	sprintf(str, "0x%lx", data_addr);
	argv[0] = "bootm";
	argv[1] = str;

	return do_bootm(find_cmd("do_bootm"), 0, 2, argv);
}

static int do_mtkload(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	char s_load_addr[CONFIG_SYS_CBSIZE + 1];
	char *addr_end, *def_load_addr = NULL;
	ulong data_load_addr;
	size_t data_size = 0;

	printf("\n");
	cprintln(PROMPT, "*** Loading image ***");
	printf("\n");

	/* Set load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
	def_load_addr = __stringify(CONFIG_SYS_LOAD_ADDR);
#elif defined(CONFIG_LOADADDR)
	def_load_addr = __stringify(CONFIG_LOADADDR);
#endif

	if (env_update("loadaddr", def_load_addr, "Input load address:",
		       s_load_addr, sizeof(s_load_addr)))
		return CMD_RET_FAILURE;

	data_load_addr = simple_strtoul(s_load_addr, &addr_end, 0);
	if (*addr_end) {
		printf("\n");
		cprintln(ERROR, "*** Invalid load address! ***");
		return CMD_RET_FAILURE;
	}

	printf("\n");

	/* Load data */
	if (load_data(data_load_addr, &data_size, "bootfile"))
		return CMD_RET_FAILURE;

	printf("\n");
	cprintln(PROMPT, "*** Loaded %zd (0x%zx) bytes at 0x%08lx ***",
		 data_size, data_size, data_load_addr);
	printf("\n");

	image_load_addr = data_load_addr;

	/* Whether to run or not */
	if (confirm_yes("Run loaded data now? (Y/n):")) {
		/* Run image */
		return run_image(data_load_addr, data_size);
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkload, 1, 0, do_mtkload, "MTK image loading utility", NULL);
