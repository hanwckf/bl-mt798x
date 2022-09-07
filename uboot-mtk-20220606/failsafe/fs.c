/* SPDX-License-Identifier:	GPL-2.0 */
/*
 * Copyright (C) 2019 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 */

#include <common.h>

#include "fs.h"

#include "httpd-fsdata.c"

const struct fs_desc *fs_find_file(const char *path)
{
#if 0
	struct fs_desc *start = ll_entry_start(struct fs_desc, fs);
	int len = ll_entry_count(struct fs_desc, fs);

	while (len) {
		if (!strcmp(start->path, path))
			return start;

		len--;
		start++;
	}
#endif
	int i;

	for(i = 0; i < HTTPD_FS_NUMFILES; i++) {
		struct fs_desc *start = &httpd_filesystem[i];

		if(!strncmp(path, &start->path[1], strlen(path)))
		   return start;
	}

	return NULL;
}
