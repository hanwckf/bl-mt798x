// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image booting command
 */

#include <command.h>
#include <linux/types.h>

#include "boot_helper.h"

static int do_mtkboardboot(struct cmd_tbl *cmdtp, int flag, int argc,
			   char *const argv[])
{
	int ret;

	ret = board_boot_default();
	if (ret)
		return CMD_RET_FAILURE;

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkboardboot, 1, 0, do_mtkboardboot,
	   "Boot MTK firmware", ""
);
