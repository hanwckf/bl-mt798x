/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Helper to print colored texts
 */

#ifndef _COLORED_PRINT_H_
#define _COLORED_PRINT_H_

#include <stdio.h>

#define COLOR_PROMPT	"\x1b[0;33m"
#define COLOR_INPUT	"\x1b[4;36m"
#define COLOR_ERROR	"\x1b[93;41m"
#define COLOR_CAUTION	"\x1b[1;31m"
#define COLOR_NORMAL	"\x1b[0m"

#define cprintln(color, fmt, ...) \
	printf(COLOR_##color fmt COLOR_NORMAL "\n", ##__VA_ARGS__)

#define cprint(color, fmt, ...) \
	printf(COLOR_##color fmt COLOR_NORMAL, ##__VA_ARGS__)

#define cprint_cont(color, fmt, ...) \
	printf(COLOR_##color fmt, ##__VA_ARGS__)

#endif /* _COLORED_PRINT_H_ */
