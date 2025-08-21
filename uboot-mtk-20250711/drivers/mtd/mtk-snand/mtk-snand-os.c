// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include "mtk-snand-def.h"

int mtk_snand_log(struct mtk_snand_plat_dev *pdev,
		  enum mtk_snand_log_category cat, const char *fmt, ...)
{
	const char *catname = "";
	va_list ap;
	int ret;

	switch (cat) {
	case SNAND_LOG_NFI:
		catname = "NFI: ";
		break;
	case SNAND_LOG_SNFI:
		catname = "SNFI: ";
		break;
	case SNAND_LOG_ECC:
		catname = "ECC: ";
		break;
	default:
		break;
	}

	puts("SPI-NAND: ");
	puts(catname);

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

	return ret;
}
