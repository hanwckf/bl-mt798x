// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 */

#ifndef _PLAT_EFUSE_H_
#define _PLAT_EFUSE_H_

#include <stdint.h>

int plat_efuse_sbc_init(void);
uint32_t efuse_get_len(uint32_t field, uint32_t *len);
uint32_t efuse_validate_field(uint32_t field, u_register_t flags);
uint32_t efuse_disable(uint32_t field);

#endif /* _PLAT_EFUSE_H_ */
