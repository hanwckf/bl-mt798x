// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <lib/mmio.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <plat/common/platform.h>
#include <plat/common/common_def.h>
#include <plat_private.h>
#include <mcucfg.h>
#include <mtspmc.h>
#include <cpuxgpt.h>
#include <emi_mpu.h>
#include <devapc.h>
#include <mtk_efuse.h>

#if ENCRYPT_BL33
#include <bl33_dec.h>
#endif

#ifdef MTK_FW_ENC
#include <fw_dec.h>
#endif

static void mt7987_disable_l2c_shared(void)
{
	uint32_t inval_l2_tags_complete;

	dsb();
	isb();
	/* Flush and invalidate data cache */
	dcsw_op_all(DCCISW);
	dsb();
	isb();

	mmio_setbits_32(mcucfg_reg(MCUCFG_L2CTAG), BIT(29));
	do {
		inval_l2_tags_complete = mmio_read_32(mcucfg_reg(MCUCFG_L2CTAG));
		inval_l2_tags_complete &= BIT(30);

	} while (!inval_l2_tags_complete);
	mmio_clrbits_32(mcucfg_reg(MCUCFG_L2CTAG), BIT(29));
	mmio_write_32(mcucfg_reg(MCUCFG_L2CCFG), 0x300);
}

static void platform_setup_sram(void)
{
	/* change shared sram back to L2C */
	uint32_t l2c_cfg_mp0 = mmio_read_32(mcucfg_reg(MCUCFG_L2CCFG));
	uint32_t mp0_l2c_size_cfg = ((l2c_cfg_mp0 & 0xf00) >> 8);

	if (l2c_cfg_mp0 & BIT(0)) {
		switch (mp0_l2c_size_cfg) {
		case 0x3:
			INFO("MCUSYS: Disable 512KB L2C shared SRAM\n");
			mt7987_disable_l2c_shared();
			break;
		case 0x1:
			INFO("MCUSYS: Disable 256KB L2C shared SRAM\n");
			mt7987_disable_l2c_shared();
			break;
		default:
			break;
		}
	}
}

static void plat_efuse_init(void)
{
#if TRUSTED_BOARD_BOOT
	plat_efuse_sbc_init();
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
	platform_setup_sram();
	spmc_init();

	/* enable mmu */
	plat_configure_mmu_el3(BL_CODE_BASE, BL_COHERENT_RAM_END - BL_CODE_BASE,
			       BL_CODE_BASE, BL_CODE_END, BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
}
