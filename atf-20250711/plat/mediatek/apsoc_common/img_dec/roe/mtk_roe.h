/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef MTK_ROE_H
#define MTK_ROE_H

#include <stdint.h>
#include <stddef.h>

#define ROE_INVALID_PARAM_ERR		1
#define ROE_INIT_KEY_ERR		2
#define ROE_GET_PLAT_KEY_ERR		3
#define ROE_DERIVE_KEY_ERR		4

int set_roe_key_next_bl_params(uint64_t *key_arg, uint64_t *len_arg);

int init_roe_key(uint8_t *key, size_t key_len);

void reset_roe_key(void);

int derive_from_roe_key(uint8_t *salt, size_t salt_len,
			uint8_t *key, size_t key_len);

int check_key_is_zero(uint8_t *key, size_t key_len);

#endif /* MTK_ROE_H */
