/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 */

#ifndef MTK_PLAT_KEY_H
#define MTK_PLAT_KEY_H

#include <stdint.h>

void disable_plat_key(void);

int get_plat_key(uint8_t *plat_key, size_t key_len);

#endif /* MTK_PLAT_KEY_H */
