/*
 * Copyright (C) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MTK_HUK_H_
#define _MTK_HUK_H_

#define MTK_HUK_HUID_FIELDS			2
#define MTK_HUK_HUID_LEN			8
#define MTK_HUK_KEY_LEN				16
#define MTK_HUK_DER_KEY_LEN			32

#define MTK_HUK_SUCC				U(0)
#define MTK_HUK_ERR_INVAL			U(1)
#define MTK_HUK_ERR_EFUSE			U(2)
#define MTK_HUK_ERR_SEJ				U(3)
#define MTK_HUK_ERR_DERIVED			U(3)

uint32_t mtk_get_huk_info(uint8_t *key_ptr, size_t *key_len, size_t size);

uint32_t mtk_get_derk_info(uint8_t *key_ptr, size_t *key_len, size_t size,
			   const uint8_t *salt, size_t salt_len,
			   const uint8_t *info, size_t info_len);
#endif /* _MTK_HUK_H_ */
