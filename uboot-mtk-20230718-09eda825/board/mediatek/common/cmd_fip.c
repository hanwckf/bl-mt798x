// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <common.h>
#include <command.h>
#include "colored_print.h"
#include "fip_helper.h"
#include "fip.h"

#define DECLARE_FIP_SUB_CMD(sub_cmd, min_argc, max_argc, fip_func) \
	static int do_fip_##sub_cmd##_cmd(struct cmd_tbl *cmdtp, int flag, \
					  int argc, char *const argv[]) \
	{ \
		argc--; \
		argv++; \
		if (argc < (min_argc)) \
			return CMD_RET_USAGE; \
		if (argc > (max_argc)) \
			return CMD_RET_USAGE; \
		return fip_func(argc, argv); \
	}

#define FIP_SUB_CMD(sub_cmd, usage, help) \
	U_BOOT_CMD_MKENT(sub_cmd, CONFIG_SYS_MAXARGS, 0, \
			 do_fip_##sub_cmd##_cmd, usage, help)

static bool has_working_fip = 0;
static struct fip working_fip;

enum fip_cmd {
	FIP_ADDR,
	FIP_INFO,
	FIP_UNPACK,
	FIP_UPDATE,
	FIP_CREATE,
	FIP_REPACK,
	FIP_CHECK_UBOOT,
};

static int do_fip_addr(int argc, char *const argv[])
{
	const char *str_fip_addr, *str_fip_size;
	size_t size = MAX_FIP_SIZE;
	size_t addr, fip_size;
	int ret;

	if (has_working_fip) {
		free_fip(&working_fip);
		has_working_fip = 0;
	}

	str_fip_addr = *argv;
	argc--;
	argv++;

	if (argc > 0) {
		str_fip_size = *argv;
		fip_size = simple_strtoul(str_fip_size, NULL, 0);
		if (fip_size) {
			size = fip_size;
		} else {
			cprintln(ERROR, "*** Wrong FIP size ***");
			return CMD_RET_FAILURE;
		}
	}

	addr = simple_strtoul(str_fip_addr, NULL, 0);
	if (!addr) {
		cprintln(ERROR, "*** Wrong FIP address ***");
		return CMD_RET_FAILURE;
	}

	ret = init_fip((const void *)addr, size, &working_fip);
	if (ret) {
		cprintln(ERROR, "*** FIP initialization failed (%d) ***", ret);
		return CMD_RET_FAILURE;
	}

	has_working_fip = 1;

	return CMD_RET_SUCCESS;
}

static int get_fip(struct fip **fip)
{
	if (!has_working_fip) {
		printf("No FIP memory address configured, Please configure\n"
		       "the FIP address via \"fip addr <address>\" command.\n");
		cprintln(ERROR, "*** Aborting! ***");
		return CMD_RET_FAILURE;
	}
	*fip = &working_fip;

	return 0;
}

static int do_fip_info(int argc, char *const argv[])
{
	const char *image_name;
	struct fip *fip;
	int ret;

	ret = get_fip(&fip);
	if (ret)
		return ret;

	if (argc > 0) {
		image_name = *argv;
		ret = fip_show_image(fip, image_name);
		if (ret)
			return CMD_RET_FAILURE;
	} else {
		fip_show(fip);
	}

	return CMD_RET_SUCCESS;
}

static int do_fip_unpack(int argc, char *const argv[])
{
	const char *str_image, *str_out_addr, *str_max_size;
	size_t size = MAX_FIP_SIZE;
	size_t addr_out, max_size, out_size;
	const char *image_name;
	struct fip *fip;
	int ret;

	ret = get_fip(&fip);
	if (ret)
		return ret;

	str_image = *argv;
	argc--;
	argv++;

	image_name = str_image;

	str_out_addr = *argv;
	argc--;
	argv++;

	addr_out = simple_strtoul(str_out_addr, NULL, 0);
	if (!addr_out) {
		cprintln(ERROR, "*** Wrong image output address ***");
		return CMD_RET_USAGE;
	}

	if (argc > 0) {
		str_max_size = *argv;
		max_size = simple_strtoul(str_max_size, NULL, 0);
		if (max_size) {
			size = max_size;
		} else {
			cprintln(ERROR, "*** Wrong image max_size ***");
			return CMD_RET_USAGE;
		}
	}

	ret = fip_unpack(fip, image_name, (void *)addr_out, size, &out_size);

	if (ret) {
		cprintln(ERROR, "*** FIP unpack failed (%d) ***", ret);
		return CMD_RET_FAILURE;
	}

	printf("unpack '%s' image to 0x%lx, size = %ld (%lx hex)\n", image_name,
	       addr_out, out_size, out_size);

	return CMD_RET_SUCCESS;
}

static int do_fip_update(int argc, char *const argv[])
{
	const char *str_image, *str_in_addr, *str_size;
	const char *image_name;
	size_t addr_in, size;
	struct fip *fip;
	int ret;

	ret = get_fip(&fip);
	if (ret)
		return ret;

	str_image = *argv;
	argc--;
	argv++;

	image_name = str_image;

	str_in_addr = *argv;
	argc--;
	argv++;

	addr_in = simple_strtoul(str_in_addr, NULL, 0);
	if (!addr_in) {
		cprintln(ERROR, "*** Wrong image output address ***");
		return CMD_RET_USAGE;
	}

	str_size = *argv;
	argc--;
	argv++;

	size = simple_strtoul(str_size, NULL, 0);
	if (!size) {
		cprintln(ERROR, "*** Wrong image size ***");
		return CMD_RET_USAGE;
	}

	ret = fip_update(fip, image_name, (void *)addr_in, size);

	if (ret) {
		cprintln(ERROR, "*** FIP update failed (%d) ***", ret);
		return CMD_RET_FAILURE;
	}

	fip_show(fip);

	return CMD_RET_SUCCESS;
}

static int do_fip_create(int argc, char *const argv[])
{
	const char *str_out_addr, *str_max_size;
	size_t size = MAX_FIP_SIZE;
	size_t addr_out, max_size, out_size;
	struct fip *fip;
	int ret;

	ret = get_fip(&fip);
	if (ret)
		return ret;

	str_out_addr = *argv;
	argc--;
	argv++;

	addr_out = simple_strtoul(str_out_addr, NULL, 0);
	if (!addr_out) {
		cprintln(ERROR, "*** Wrong image output address ***");
		return CMD_RET_USAGE;
	}

	if (argc > 0) {
		str_max_size = *argv;
		max_size = simple_strtoul(str_max_size, NULL, 0);
		if (max_size) {
			size = max_size;
		} else {
			cprintln(ERROR, "*** Wrong image max_size ***");
			return CMD_RET_USAGE;
		}
	}

	ret = fip_create(fip, (void *)addr_out, size, &out_size);

	if (ret) {
		cprintln(ERROR, "*** FIP repack failed (%d) ***", ret);
		return CMD_RET_FAILURE;
	}

	printf("repack fip image to 0x%lx, size = %ld (%lx hex)\n", addr_out,
	       out_size, out_size);

	return CMD_RET_SUCCESS;
}

static int do_fip_repack(int argc, char *const argv[])
{
	const char *str_swap_addr, *str_max_size;
	size_t size = MAX_FIP_SIZE;
	size_t addr_swap, max_size;
	struct fip *fip;
	int ret;

	ret = get_fip(&fip);
	if (ret)
		return ret;

	str_swap_addr = *argv;
	argc--;
	argv++;

	addr_swap = simple_strtoul(str_swap_addr, NULL, 0);
	if (!addr_swap) {
		cprintln(ERROR, "*** Wrong image swap address ***");
		return CMD_RET_USAGE;
	}

	if (argc > 0) {
		str_max_size = *argv;
		max_size = simple_strtoul(str_max_size, NULL, 0);
		if (max_size) {
			size = max_size;
		} else {
			cprintln(ERROR, "*** Wrong image max_size ***");
			return CMD_RET_USAGE;
		}
	}

	ret = fip_repack(fip, (void *)addr_swap, size);
	if (ret) {
		cprintln(ERROR, "*** FIP repack failed (%d) ***", ret);
		return CMD_RET_FAILURE;
	}

	return CMD_RET_SUCCESS;
}

static int do_fip_check_uboot(int argc, char *const argv[])
{
	const void *uboot_data;
	size_t uboot_size;
	struct fip *fip;
	int ret;

	if (argc > 0) {
		ret = do_fip_addr(argc, argv);
		if (ret)
			return ret;
	}

	ret = get_fip(&fip);
	if (ret)
		return ret;

	ret = fip_get_image(fip, "u-boot", &uboot_data, &uboot_size);
	if (ret) {
		cprintln(ERROR, "*** U-boot image not found (%d) ***", ret);
		return CMD_RET_FAILURE;
	}

	if (!check_uboot_data(uboot_data, uboot_size)) {
		cprintln(ERROR, "*** FIP check failed ***");
		return CMD_RET_FAILURE;
	}

	printf("*** FIP check passed ***\n");

	return CMD_RET_SUCCESS;
}

DECLARE_FIP_SUB_CMD(addr, 1, 2, do_fip_addr);
DECLARE_FIP_SUB_CMD(info, 0, 1, do_fip_info);
DECLARE_FIP_SUB_CMD(show, 0, 1, do_fip_info);
DECLARE_FIP_SUB_CMD(print, 0, 1, do_fip_info);
DECLARE_FIP_SUB_CMD(unpack, 2, 3, do_fip_unpack);
DECLARE_FIP_SUB_CMD(update, 3, 3, do_fip_update);
DECLARE_FIP_SUB_CMD(create, 1, 2, do_fip_create);
DECLARE_FIP_SUB_CMD(repack, 1, 2, do_fip_repack);
DECLARE_FIP_SUB_CMD(check, 0, 2, do_fip_check_uboot);

static struct cmd_tbl fip_cmd_sub[] = {
	FIP_SUB_CMD(addr, "", ""),
	FIP_SUB_CMD(info, "", ""),
	FIP_SUB_CMD(show, "", ""),
	FIP_SUB_CMD(print, "", ""),
	FIP_SUB_CMD(unpack, "", ""),
	FIP_SUB_CMD(update, "", ""),
	FIP_SUB_CMD(create, "", ""),
	FIP_SUB_CMD(repack, "", ""),
	FIP_SUB_CMD(check, "", ""),
};

int do_fip(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct cmd_tbl *cp;

	argc--;
	argv++;

	cp = find_cmd_tbl(argv[0], fip_cmd_sub, ARRAY_SIZE(fip_cmd_sub));

	if (cp)
		return cp->cmd(cmdtp, flag, argc, argv);

	return CMD_RET_USAGE;
}

static char fip_help_text[] =
	"addr <address> [<size>]     - set fip image address to operate\n"
	"fip info [<image>]              - show fip image related information\n"
	"fip unpack <image> <addr_out> [<max_size>] - unpack specified image\n"
	"                                  in the fip image to the target\n"
	"                                  address\n"
	"fip update <image> <addr_in> <size> - update specified fip toc\n"
	"                                  entry in the fip image\n"
	"fip create <addr_out> [<max_size>] - create new fip image on target\n"
	"                                  address\n"
	"fip repack <addr_swap> [<max_size>] - repack updated fip image\n"
	"fip check [<addr>] [<size>]     - check whether fip image is valid\n"
	"                                  for this u-boot\n";

#if CONFIG_IS_ENABLED(AUTO_COMPLETE)

static int fip_complete(int argc, char *const argv[], char last_char, int maxv,
			char *cmdv[])
{
	int r = 0;
	int i;

	if (argc > 2)
		return 0;

	for (i = 0; i < ARRAY_SIZE(fip_cmd_sub); i++) {
		if (argc == 2) {
			if (last_char == ' ')
				break;
			if (strncmp(fip_cmd_sub[i].name, argv[1],
			    strlen(argv[1])))
				continue;
		}

		cmdv[r++] = fip_cmd_sub[i].name;
	}

	cmdv[r] = NULL;

	return r;
}

U_BOOT_CMD_COMPLETE(fip, CONFIG_SYS_MAXARGS, 0, do_fip, "FIP utility commands",
		    fip_help_text, fip_complete);

#else

U_BOOT_CMD(fip, CONFIG_SYS_MAXARGS, 0, do_fip, "FIP utility commands",
	   fip_help_text);

#endif
