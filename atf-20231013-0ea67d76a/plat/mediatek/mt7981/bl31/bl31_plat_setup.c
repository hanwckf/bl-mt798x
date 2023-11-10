/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <lib/mmio.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <plat/common/platform.h>
#include <plat/common/common_def.h>
#include <plat_private.h>
#include <mcucfg.h>
#include <mtspmc.h>
#include <mtk_gic_v3.h>
#include <timer.h>

static void mt7981_disable_l2c_shared(void)
{
	uint32_t inval_l2_tags_complete;

	dsb();
	isb();
	/* Flush and invalidate data cache */
	dcsw_op_all(DCCISW);
	dsb();
	isb();

	mmio_setbits_32((uintptr_t)&mt7981_mcucfg->mp0_misc_config3, BIT(29));
	do {
		inval_l2_tags_complete = mmio_read_32(
			(uintptr_t)&mt7981_mcucfg->mp0_misc_config3);
		inval_l2_tags_complete &= BIT(30);

	} while (!inval_l2_tags_complete);
	mmio_clrbits_32((uintptr_t)&mt7981_mcucfg->mp0_misc_config3, BIT(29));
	mmio_write_32((uintptr_t)&mt7981_mcucfg->l2c_cfg_mp0, 0x100);
}

static void platform_setup_sram(void)
{
	/* change shared sram back to L2C */
	uint32_t l2c_cfg_mp0 = mmio_read_32(
			(uintptr_t)&mt7981_mcucfg->l2c_cfg_mp0);
	uint32_t mp0_l2c_size_cfg = ((l2c_cfg_mp0 & 0xf00) >> 8);

	INFO("Total CPU count: %d\n", PLATFORM_CORE_COUNT);

	if (l2c_cfg_mp0 & BIT(0)) {
		switch (mp0_l2c_size_cfg) {
		case 0x3:
			INFO("MCUSYS: L2C share 512K --> 0K\n");
			mt7981_disable_l2c_shared();
			break;
		case 0x1:
			INFO("MCUSYS: L2C share 256K --> 0K\n");
			mt7981_disable_l2c_shared();
			break;
		default:
			break;
		}
	}

	/*
	 * NETSYS SRAM remapping CR (0x1A020018) not in mmu range
	 * DO NOT R/W it after BL31 mmu init
	 */
	INFO("Disable NETSYS SRAM remapping\n");
	mmio_write_32((uintptr_t)0x1A020018, 0x0);
}

/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/
void bl31_platform_setup(void)
{
	mtk_timer_init();
	generic_delay_timer_init();

	/* Initialize the gic cpu and distributor interfaces */
	plat_mt_gic_init();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
	platform_setup_sram();

	spmc_init();

	/* enable mmu */
	plat_configure_mmu_el3(BL_CODE_BASE,
			       BL_COHERENT_RAM_END - BL_CODE_BASE,
			       BL_CODE_BASE,
			       BL_CODE_END,
			       BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
}
