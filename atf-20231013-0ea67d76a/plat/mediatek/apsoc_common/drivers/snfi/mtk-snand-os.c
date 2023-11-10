// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common/debug.h>
#include <plat/common/platform.h>

#include "mtk-snand-def.h"

int mtk_snand_log(struct mtk_snand_plat_dev *pdev,
		  enum mtk_snand_log_category cat, const char *fmt, ...)
{
	const char *prefix_str, *catname = "";
	int log_level = LOG_LEVEL_NOTICE;
	va_list ap;
	int ret;

	switch (cat) {
	case SNAND_LOG_NFI:
		catname = "NFI";
		break;
	case SNAND_LOG_SNFI:
		catname = "SNFI";
		break;
	case SNAND_LOG_ECC:
		catname = "ECC";
		break;
	default:
		break;
	}

	if (log_level > LOG_LEVEL)
		return 0;

	prefix_str = plat_log_get_prefix(log_level);

	while (*prefix_str != '\0') {
		(void)putchar(*prefix_str);
		prefix_str++;
	}

	if (*catname)
		printf("SPI-NAND: %s: ", catname);
	else
		printf("SPI-NAND: ");

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

	return ret;
}
