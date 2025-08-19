// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <common/bl_common.h>
#include <bl2pl/bl2pl.h>
#include <platform_def.h>
#include <string.h>
#include <tf_unxz.h>

#if defined(XZ_SIMPLE_PRINT_ERROR)
#define BL2PL_ERR		xz_simple_puts
#else
#define BL2PL_ERR		ERROR
#endif

extern uintptr_t __COPY_SIZE__;

static uint8_t xz_pool[0x6f50 + 0x4d8 + 0x50 + 1024];

#pragma weak bl2pl_platform_setup

void bl2pl_platform_setup(void)
{
}

int bl2pl_main(uintptr_t base_addr)
{
	uintptr_t payload_addr, payload_new_addr, load_addr, outbuf;
	uint32_t payload_size, bl2pl_size;
	struct bl2pl_hdr *payload_hdr;
	void (*pfunc)(void);
	int ret;

	bl2pl_platform_setup();

#if !defined(XZ_SIMPLE_PRINT_ERROR)
	NOTICE("BL2PL: %s\n", version_string);
	NOTICE("BL2PL: %s\n", build_message);
	NOTICE("\n");
#endif

	/* Locate the payload */
	bl2pl_size = (uintptr_t)&__COPY_SIZE__;
	payload_addr = base_addr + bl2pl_size;

	/* Validate payload */
	payload_hdr = (void *)payload_addr;
	if (payload_hdr->magic != BL2PL_HDR_MAGIC) {
		BL2PL_ERR("Invalid BL2 payload header\n");
		goto fail;
	}

	if (payload_hdr->load_addr < L2_SRAM_BASE ||
	    payload_hdr->load_addr >= L2_SRAM_BASE + L2_SRAM_SIZE) {
		BL2PL_ERR("Invalid BL2 payload load address\n");
		goto fail;
	}

	if (payload_hdr->size > L2_SRAM_SIZE / 2) {
		BL2PL_ERR("BL2 payload is too large\n");
		goto fail;
	}

	/* Calculate new compressed payload address */
	payload_new_addr = L2_SRAM_BASE + L2_SRAM_SIZE - payload_hdr->size;
	payload_new_addr &= ~(0x10 - 1);

	/* Move payload to end of SRAM */
	memmove((void *)payload_new_addr,
		(void *)payload_addr + sizeof(struct bl2pl_hdr),
		payload_hdr->size);

	/* Save payload information */
	payload_size = payload_hdr->size;
	load_addr = payload_hdr->load_addr;

	/* Decompress payload */
	outbuf = load_addr;

	ret = unxz(&payload_new_addr, payload_size, &outbuf,
		   L2_SRAM_SIZE - payload_size, (uintptr_t)&xz_pool,
		   sizeof(xz_pool));

	if (!ret) {
		/* Jump to payload directly */
		pfunc = (void (*)(void))load_addr;
		(*pfunc)();
	}

fail:
	while (1);
}
