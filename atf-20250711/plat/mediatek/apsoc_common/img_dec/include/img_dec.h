/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 */

#ifndef IMG_DEC_H
#define IMG_DEC_H

void img_dec_set_next_bl_params(uint64_t *key_arg, uint64_t *len_arg);

void img_dec_bl_params_init(uint8_t *key, size_t key_len);

void img_dec_runtime_setup(void);

#endif /* IMG_DEC_H */
