/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Wget command using mtk_wget
 */

#include <command.h>
#include <errno.h>
#include <vsprintf.h>
#include "mtk_wget.h"
#include "load_data.h"

static int do_wget(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	ulong data_load_addr;
	char *url;
	int ret;

	if (argc == 1)
		return CMD_RET_USAGE;

	if (argc > 2) {
		data_load_addr = hextoul(argv[1], NULL);
		url = argv[2];
	} else {
		data_load_addr = get_load_addr();
		url = argv[1];
	}

	ret = start_wget(data_load_addr, url, NULL);
	if (ret)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(wget, 3, 0, do_wget,
	"Download file from HTTP server",
	"[loadaddr] <url>"
);
