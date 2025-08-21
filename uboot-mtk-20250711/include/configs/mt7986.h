/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Configuration for MediaTek MT7986 SoC
 *
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef __MT7986_H
#define __MT7986_H

/* Extra environment variables */
#ifdef CONFIG_MTK_DEFAULT_FIT_BOOT_CONF
#define FIT_BOOT_CONF_ENV	"bootconf=" CONFIG_MTK_DEFAULT_FIT_BOOT_CONF "\0"
#else
#define FIT_BOOT_CONF_ENV
#endif

#define CFG_EXTRA_ENV_SETTINGS	\
	FIT_BOOT_CONF_ENV

#endif
