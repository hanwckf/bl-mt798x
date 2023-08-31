// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <common.h>
#include <command.h>
#include "bl2_helper.h"
#include "colored_print.h"

enum bl2_cmd {
	BL2_CHECK_IMAGE,
};

static int do_bl2_check_image(int argc, char *const argv[])
{
	const char *str_bl2_addr, *str_bl2_size;
	ulong addr, size;

	str_bl2_addr = *argv;
	argc--;
	argv++;

	if (argc > 0) {
		str_bl2_size = *argv;
		size = simple_strtoul(str_bl2_size, NULL, 0);
		if (!size)
			size = MAX_BL2_SIZE;
	}

	addr = simple_strtoul(str_bl2_addr, NULL, 0);
	if (addr) {
		if (!bl2_check_image_data((void *)addr, size)) {
			cprintln(NORMAL, "*** BL2 verification passed ***");
			return CMD_RET_SUCCESS;
		}
		cprintln(ERROR, "*** BL2 verification failed ***");
	}

	return CMD_RET_FAILURE;
}

int do_bl2(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	const char *str_cmd;
	enum bl2_cmd sub_cmd;
	int ret;

	if (argc < 2)
		return CMD_RET_USAGE;

	str_cmd = argv[1];
	argc -= 2;
	argv += 2;

	switch (*str_cmd) {
	case 'c':
		sub_cmd = BL2_CHECK_IMAGE;
		if (argc < 1)
			return CMD_RET_USAGE;
		break;
	default:
		return CMD_RET_USAGE;
	}

	switch (sub_cmd) {
	case BL2_CHECK_IMAGE:
		ret = do_bl2_check_image(argc, argv);
		break;
	default:
		return CMD_RET_USAGE;
	}

	return ret;
}

U_BOOT_CMD(bl2, CONFIG_SYS_MAXARGS, 0, do_bl2, "BL2 utility commands", "\n"
	"check <addr> [length] - check bl2 image is valid for this u-boot or not\n"
);
