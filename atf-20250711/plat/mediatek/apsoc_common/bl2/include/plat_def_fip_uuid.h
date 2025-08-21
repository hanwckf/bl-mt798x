// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 */

#ifndef PLAT_DEF_FIP_UUID_H
#define PLAT_DEF_FIP_UUID_H

#define UUID_MTK_FIP_CHECKSUM \
	{{0xa2, 0xcc, 0xea, 0xb7}, {0xf8, 0x25}, {0x4b, 0x27}, 0x97, 0x04, \
	 {0x63, 0x3a, 0x6f, 0xd6, 0x9a, 0xd8} }

struct mtk_fip_checksum {
	unsigned int len;
	unsigned int crc;
	unsigned char sha256sum[0x20];
};

#endif /* PLAT_DEF_FIP_UUID_H */
