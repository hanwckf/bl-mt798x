/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data upgrading helper
 */

#ifndef _LOAD_DATA_H_
#define _LOAD_DATA_H_

#include <linux/types.h>

int env_update(const char *varname, const char *defval, const char *prompt,
	       char *buffer, size_t bufsz);
int confirm_yes(const char *prompt);

int load_data(ulong addr, size_t *data_size, const char *env_name);

#endif /* _LOAD_DATA_H_ */
