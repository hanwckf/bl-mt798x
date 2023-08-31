/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 Mediatek Inc. All Rights Reserved.
 */
#ifndef _MTK_FSEK_H_
#define _MTK_FSEK_H_

#define MTK_FSEK_ROE_KEY			"roek"
#define MTK_FSEK_RFS_KEY			"rfsk"
#define MTK_FSEK_FIT_SECRET			"fit-secret"
#define MTK_FSEK_SIG_HASH			"signature-hash"
#define FIT_SECRETS_PATH			"/fit-secrets"
#define FDT_DEV_NODE				"key_dev_specific"
#define MAX_BOOTARGS_LEN			4096
#define MTK_FSEK_GET_SHM_CONFIG			0xC2000520
#define MTK_FSEK_DECRYPT_RFSK			0xC2000521
#define MTK_FSEK_GET_KEY			0xC2000522
#define MTK_FSEK_MAX_DATA_LEN			160
#define MTK_FSEK_KEY_LEN			32
#define MTK_FSEK_RFS_KEY_ID			0
#define MTK_FSEK_DER_KEY_ID			1

int mtk_fsek_decrypt_rfsk(const void *fit, const char *conf_name,
			  int conf_noffset);

int mtk_fsek_set_fdt(void *fdt);

#endif /* _MTK_FSEK_H_ */
