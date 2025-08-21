/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <drivers/generic_delay_timer.h>
#include <plat/common/platform.h>
#include <plat_private.h>
#include <mcucfg.h>
#include <cpuxgpt.h>
#include <mtk_efuse.h>

static void platform_setup_cpu(void)
{
	/* set LITTLE cores arm64 boot mode */
	mmio_setbits_32((uintptr_t)&mt7622_mcucfg->mp0_misc_config[3],
			MP0_CPUCFG_64BIT);
}

static void platform_setup_sram(void)
{
	/* protect BL31 memory from non-secure read/write access */
	mmio_write_32(SRAMROM_SEC_ADDR,
		      (uint32_t)(BL31_END + 0x3ff) & 0x3fc00);
	mmio_write_32(SRAMROM_SEC_CTRL, 0x10000ff9);
}

static void plat_efuse_init(void)
{
#if TRUSTED_BOARD_BOOT
	plat_efuse_sbc_init();
#endif
#if ANTI_ROLLBACK
	plat_efuse_ar_init();
#endif
}

/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/
void bl31_platform_setup(void)
{
	platform_setup_cpu();
	platform_setup_sram();

	generic_delay_timer_init();

	plat_efuse_init();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	plat_cci_init();
	plat_cci_enable();

	plat_configure_mmu_el3(BL_CODE_BASE,
			       BL_COHERENT_RAM_END - BL_CODE_BASE,
			       BL_CODE_BASE,
			       BL_CODE_END,
			       BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
}
