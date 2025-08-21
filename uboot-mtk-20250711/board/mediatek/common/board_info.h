/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef _BOARD_INFO_H_
#define _BOARD_INFO_H_

#define MAX_COMPAT_COUNT	10

struct compat_list {
	unsigned int count;
	const char *compats[MAX_COMPAT_COUNT];
};

void print_compat_list(const struct compat_list *cl);
int fdt_read_compat_list(const void *fdt_blob, ulong offset, const char *name,
			 struct compat_list *cl);

#endif /* _BOARD_INFO_H_ */
