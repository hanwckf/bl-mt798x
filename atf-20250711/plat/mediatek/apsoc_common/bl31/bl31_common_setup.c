// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <stddef.h>
#include <platform_def.h>
#include <hsuart.h>

#ifdef MTK_IMG_ENC
#include <img_dec.h>
#endif

static size_t dram_size;

size_t mtk_bl31_get_dram_size(void)
{
	return dram_size;
}

/*******************************************************************************
 * Perform any BL3-1 early platform setup. Here is an opportunity to copy
 * parameters passed by the calling EL (S-EL1 in BL2 & EL3 in BL1) before they
 * are lost (potentially). This needs to be done before the MMU is initialized
 * so that the memory layout can be used while creating page tables.
 * BL2 has flushed this information to memory, so we are guaranteed to pick up
 * good data.
 ******************************************************************************/
void bl31_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				u_register_t arg2, u_register_t arg3)
{
	static console_t console;

	dram_size = arg1;

	console_hsuart_register(UART_BASE, UART_CLOCK, UART_BAUDRATE, true,
				&console);
#ifdef ENABLE_BL31_RUNTIME_LOG
	console.flags |= CONSOLE_FLAG_RUNTIME;
#endif

#ifdef MTK_IMG_ENC
	img_dec_bl_params_init((uint8_t *)arg2, (size_t)arg3);
#endif
}

void bl31_plat_runtime_setup(void)
{
#ifdef MTK_IMG_ENC
	img_dec_runtime_setup();
#endif
}
