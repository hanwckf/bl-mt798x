/*
 * Copyright (c) 2021, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef TF_UNXZ_H
#define TF_UNXZ_H

#include <stddef.h>
#include <stdint.h>

void xz_simple_puts(const char *s);

int unxz(uintptr_t *in_buf, size_t in_len, uintptr_t *out_buf, size_t out_len,
	 uintptr_t work_buf, size_t work_len);

#endif /* TF_UNXZ_H */
