/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Wget implementation using mtk_htp
 */
#ifndef _MTK_WGET_H_
#define _MTK_WGET_H_

#include <linux/types.h>

int start_wget(ulong load_addr, const char *url, size_t *retlen);

#endif /* _MTK_WGET_H_ */
