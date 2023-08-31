/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image verification helper
 */

#ifndef _VERIFY_HELPER_H_
#define _VERIFY_HELPER_H_

#include <linux/bitops.h>
#include <u-boot/sha1.h>

#include "image_helper.h"

#define FIT_HASH_CRC32		BIT(0)
#define FIT_HASH_SHA1		BIT(1)

struct fit_hashes {
	u8 sha1[SHA1_SUM_LEN];
	u32 crc32;
	int flags;
};

struct image_read_priv {
	u32 page_size;
	u32 block_size;

	int (*read)(struct image_read_priv *priv, void *buff, u64 addr,
		    size_t size);
};

bool verify_image_ram(const void *data, size_t size, u32 block_size,
		      bool verify_rootfs, struct owrt_image_info *ii,
		      struct fit_hashes *kernel_hashes,
		      struct fit_hashes *rootfs_hashes);

bool read_verify_kernel(struct image_read_priv *rpriv, void *ptr, u64 addr,
			size_t max_size, size_t *actual_size,
			struct fit_hashes *hashes);

bool read_verify_rootfs(struct image_read_priv *rpriv, const void *fit,
			void *ptr, u64 addr, u64 max_size, size_t *actual_size,
			bool keep_padding, size_t *padding_size,
			struct fit_hashes *hashes);

bool compare_hashes(const struct fit_hashes *hash1,
		    const struct fit_hashes *hash2);

#endif /* _VERIFY_HELPER_H_ */
