/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <linker_lists.h>
#include <string.h>
#include "fs.h"

const struct fs_desc *fs_find_file(const char *path)
{
	struct fs_desc *start = ll_entry_start(struct fs_desc, fs);
	int len = ll_entry_count(struct fs_desc, fs);

	while (len) {
		if (!strcmp(start->path, path))
			return start;

		len--;
		start++;
	}

	return NULL;
}
