/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#ifndef MTK_BOOT_NEXT_H
#define MTK_BOOT_NEXT_H

#include <stdint.h>

void boot_to_kernel(uint64_t pc, uint64_t r0, uint64_t r1, uint64_t aarch64);

#endif /* MTK_BOOT_NEXT_H */
