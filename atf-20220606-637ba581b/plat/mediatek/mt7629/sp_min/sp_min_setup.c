/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>

#include <platform_def.h>

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <drivers/console.h>
#include <drivers/generic_delay_timer.h>
#include <hsuart.h>
#include <lib/mmio.h>
#include <plat_private.h>
#include <plat/common/platform.h>

#include <mtk_plat_common.h>
#include <cpuxgpt.h>
#include <spmc.h>
/******************************************************************************
 * Placeholder variables for copying the arguments that have been passed to
 * BL32 from BL2.
 ******************************************************************************/
static entry_point_info_t bl33_image_ep_info;

void plat_mt_gic_driver_init(void);
void plat_mt_gic_init(void);

static void platform_setup_sram(void)
{
	/* protect BL32 memory from non-secure read/write access */
	mmio_write_32(SRAMROM_SEC_ADDR,
		      (uint32_t)(BL32_END + 0x3ff) & 0x3fc00);
	mmio_write_32(SRAMROM_SEC_CTRL, 0x10000ff9);
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * the security state specified. BL33 corresponds to the non-secure image type.
 * A NULL pointer is returned if the image does not exist.
 ******************************************************************************/
entry_point_info_t *sp_min_plat_get_bl33_ep_info(void)
{
	entry_point_info_t *next_image_info;

	next_image_info = &bl33_image_ep_info;

	if (next_image_info->pc == 0U)
		return NULL;

	return next_image_info;
}

/*******************************************************************************
 * Perform any BL32 specific platform actions.
 ******************************************************************************/
void sp_min_early_platform_setup2(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	static console_t console;

	console_hsuart_register(UART0_BASE, plat_uart_clock(), UART_BAUDRATE,
				true, &console);

	VERBOSE("sp_min_setup\n");

	/*
	 * setup mpu to protect TZRAM (0x43000000 ~ 0x43200000)
	 * disallow non-secure read and write
	 */
	mmio_write_32(0x10203160, 0x03000320);
	mmio_write_32(0x102031A0, 0x00008249);

	/*
	 * setup devapc to protect eFuse
	 * disallow non-secure write
	 */
	mmio_write_32(0x10007f00, 0x80000001);
	mmio_write_32(0x10207f00, 0x80000001);
	mmio_write_32(0x10007004, 0x00000080);

	/*
	 * setup devapc to protect devapc
	 * disallow non-secure read and write
	 */
	mmio_write_32(0x10007000, 0x00004000);

	bl_params_t *params_from_bl2 = (bl_params_t *)arg0;

	/* Imprecise aborts can be masked in NonSecure */
	write_scr(read_scr() | SCR_AW_BIT);

	assert(params_from_bl2 != NULL);
	assert(params_from_bl2->h.type == PARAM_BL_PARAMS);
	assert(params_from_bl2->h.version >= VERSION_2);

	bl_params_node_t *bl_params = params_from_bl2->head;

	/*
	 * Copy BL33 entry point information.
	 * They are stored in Secure RAM, in BL2's address space.
	 */
	while (bl_params != NULL) {
		if (bl_params->image_id == BL33_IMAGE_ID) {
			bl33_image_ep_info = *bl_params->ep_info;
			break;
		}

		bl_params = bl_params->next_params_info;
	}
}

/*******************************************************************************
 * Perform any sp_min platform setup code
 ******************************************************************************/
void sp_min_platform_setup(void)
{
	generic_delay_timer_init();
	platform_setup_sram();

	/* Initialize the gic cpu and distributor interfaces */
	plat_mt_gic_driver_init();
	plat_mt_gic_init();

	/* Power off CPU1 */
	mtk_spm_cpu1_power_off();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void sp_min_plat_arch_setup(void)
{
	plat_cci_init();
	plat_cci_enable();

	plat_configure_mmu_svc_mon(BL_CODE_BASE,
				   BL_COHERENT_RAM_END - BL_CODE_BASE,
				   BL_CODE_BASE,
				   BL_CODE_END,
				   BL_COHERENT_RAM_BASE,
				   BL_COHERENT_RAM_END);
}

void sp_min_plat_fiq_handler(uint32_t id)
{
	VERBOSE("[sp_min] interrupt #%d\n", id);
}
