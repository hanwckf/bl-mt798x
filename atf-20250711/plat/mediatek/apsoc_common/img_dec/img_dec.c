// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 */

#include <common/debug.h>
#include <mtk_roe.h>

void img_dec_set_next_bl_params(uint64_t *key_arg, uint64_t *len_arg)
{
	int ret;

	ret = set_roe_key_next_bl_params(key_arg, len_arg);
	if (ret)
		panic();
}

void img_dec_bl_params_init(uint8_t *key, size_t key_len)
{
	int ret;

	ret = init_roe_key(key, key_len);
	if (ret)
		panic();
}

void img_dec_runtime_setup(void)
{
	reset_roe_key();
}
