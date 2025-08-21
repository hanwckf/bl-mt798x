/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <drivers/generic_delay_timer.h>
#include <plat/common/platform.h>
#include <plat/common/common_def.h>
#include <plat_private.h>
#include <mcucfg.h>
#include <mtspmc.h>
#include <mtk_efuse.h>
#include <cpuxgpt.h>
#include <emi_mpu.h>
#include <devapc.h>

#if ENCRYPT_BL33
#include <bl33_dec.h>
#include <common/debug.h>
#endif

#ifdef MTK_FW_ENC
#include <fw_dec.h>
#endif

static void plat_efuse_init(void)
{
#if TRUSTED_BOARD_BOOT
	plat_efuse_sbc_init();
#endif
#if MTK_ANTI_ROLLBACK
	plat_efuse_ar_init();
#endif
}

/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/
void bl31_platform_setup(void)
{
	plat_mt_cpuxgpt_pll_update_sync();
	generic_delay_timer_init();

	emi_mpu_init();
	infra_devapc_init();

	plat_efuse_init();

#if ENCRYPT_BL33
	if (bl33_decrypt())
		panic();
#endif
#ifdef MTK_FW_ENC
	fw_dec_init();
#endif
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	spmc_init();

	/* enable mmu */
	plat_configure_mmu_el3(BL_CODE_BASE, BL_COHERENT_RAM_END - BL_CODE_BASE,
			       BL_CODE_BASE, BL_CODE_END, BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
}
