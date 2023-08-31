// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <asm/global_data.h>
#include <common.h>
#include <fdt_support.h>
#include "board_info.h"

DECLARE_GLOBAL_DATA_PTR;

void print_compat_list(const struct compat_list *cl)
{
	int i;

	for (i = 0; i < cl->count; i++) {
		printf("\"%s\"", cl->compats[i]);
		if (i < cl->count - 1)
			printf(", ");
	}

	printf("\n");
}

int fdt_read_compat_list(const void *fdt_blob, ulong offset, const char *name,
			 struct compat_list *cl)
{
	const char *compat, *end;
	const void *nodep;
	int len, ret;

	if (!offset) {
		ret = fdt_path_offset(fdt_blob, "/");
		if (ret < 0)
			return -ENOENT;
		offset = ret;
	}

	cl->count = 0;

	nodep = fdt_getprop(fdt_blob, offset, name, &len);
	if (nodep && len > 1) {
		compat = nodep;
		end = compat + len;

		while (*compat && compat < end &&
		       cl->count < ARRAY_SIZE(cl->compats)) {
			cl->compats[cl->count++] = compat;
			compat += strlen(compat) + 1;
		}
	}

	if (!cl->count)
		return -ENOENT;

	return 0;
}

int checkboard(void)
{
	struct compat_list cl;

	if (IS_ENABLED(CONFIG_OF_CONTROL))
		if (!fdt_read_compat_list(gd->fdt_blob, 0, "compatible", &cl))
			printf("       (%s)\n", cl.compats[cl.count - 1]);

	return 0;
}
