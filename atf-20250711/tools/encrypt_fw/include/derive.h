/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-CLause
 */

#ifndef DERIVE_H
#define DERIVE_H

#define DERIVE_KEY_SIZE		32

int derive_enc_key(char *plat_key, uint32_t plat_key_len,
		   char *enc_key, uint32_t enc_key_size);

#endif /* DERIVE_H */
