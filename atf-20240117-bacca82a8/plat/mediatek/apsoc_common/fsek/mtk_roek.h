/*
 * Copyright (C) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _MTK_ROEK_H_
#define _MTK_ROEK_H_

#define MTK_ROEK_KEY_NAME			"roe-key-dev-specific-derived"
#define MTK_ROEK_KEY_LEN			32

#define MTK_ROEK_SUCC				U(0)
#define MTK_ROEK_ERR_INVAL			U(1)
#define MTK_ROEK_ERR_NOT_FOUND			U(2)
#define MTK_ROEK_ERR_DEC			U(3)
#define MTK_ROEK_ERR_ENC			U(4)
#define MTK_ROEK_ERR_SBC_EN			U(5)
#define MTK_ROEK_ERR_DERIVED			U(6)

uint32_t mtk_get_roek_info_from_extern(uint8_t *key_ptr, size_t *key_len,
				       size_t size, uint8_t *cipher);

uint64_t mtk_roek_encrypt(u_register_t x1, u_register_t x2,
			  u_register_t x3, u_register_t x4,
			  void *ker_ptr, size_t size);
#endif /* _MTK_ROEK_H_ */
