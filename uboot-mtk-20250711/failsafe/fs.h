// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _FAILSAFE_FS_H_
#define _FAILSAFE_FS_H_

struct fs_desc {
	const char *path;
	size_t size;
	const void *data;
};

const struct fs_desc *fs_find_file(const char *path);

#endif /* _FAILSAFE_FS_H_ */
