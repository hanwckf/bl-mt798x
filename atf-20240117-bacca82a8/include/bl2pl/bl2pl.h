/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef BL2PL_H
#define BL2PL_H

#include <stdint.h>

#define BL2PL_HDR_MAGIC			0x50324c42	/* "BL2P" */

struct bl2pl_hdr {
	uint32_t magic;
	uint32_t load_addr;
	uint32_t size;
	uint32_t unused;
};

#endif /* BL2PL_H */
