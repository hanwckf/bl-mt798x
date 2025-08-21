// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <stdint.h>
#include <inttypes.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <common/bl_common.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <drivers/console.h>
#include <platform_def.h>
#include "mtk_boot_next.h"

static entry_point_info_t bl32_ep_info;
static entry_point_info_t bl33_ep_info;
static entry_point_info_t bl33k_ep_info;
static bool kernel_boot_once_flag;

static uint32_t mtk_default_spsr_for_bl32_entry(void)
{
	/*
	 * The Secure Payload Dispatcher service is responsible for
	 * setting the SPSR prior to entry into the BL3-2 image.
	 */
	return 0;
}

static uint32_t mtk_default_spsr_for_bl33_entry(bool kernel, bool aarch64)
{
	const char *name = kernel ? "Kernel" : "Secondary bootloader";
	uint32_t daif, spsr = 0;

	INFO("%s is %s\n", name, aarch64 ? "AArch64" : "AArch32");

	if (aarch64) {
		spsr = SPSR_64(MODE_EL2, MODE_SP_ELX, DISABLE_ALL_EXCEPTIONS);
	} else {
		daif = DAIF_ABT_BIT | DAIF_IRQ_BIT | DAIF_FIQ_BIT;
		spsr = SPSR_MODE32(MODE32_svc, SPSR_T_ARM, SPSR_E_LITTLE, daif);
	}

	return spsr;
}

void boot_to_kernel(uint64_t pc, uint64_t r0, uint64_t r1, uint64_t aarch64)
{
	uint32_t image_type;

	if (kernel_boot_once_flag)
		return;

	kernel_boot_once_flag = true;

	/* Determine which image to execute next */
	/* image_type = bl31_get_next_image_type(); */
	image_type = NON_SECURE;

	SET_PARAM_HEAD(&bl33k_ep_info, PARAM_EP, VERSION_1, 0);
	SET_SECURITY_STATE(bl33k_ep_info.h.attr, image_type);

	bl33k_ep_info.pc = pc;
	bl33k_ep_info.spsr = mtk_default_spsr_for_bl33_entry(true, !!aarch64);

	if (aarch64) {
		bl33k_ep_info.args.arg0 = r0;
		bl33k_ep_info.args.arg1 = r1;
	} else {
		bl33k_ep_info.args.arg0 = 0;
		bl33k_ep_info.args.arg1 = r0;
		bl33k_ep_info.args.arg2 = r1;
	}

	cm_init_my_context(&bl33k_ep_info);
	cm_prepare_el3_exit(image_type);
}

/*******************************************************************************
 * Return a pointer to the 'entry_point_info' structure of the next image for
 * the security state specified. BL33 corresponds to the non-secure image type
 * while BL32 corresponds to the secure image type. A NULL pointer is returned
 * if the image does not exist.
 ******************************************************************************/
entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	entry_point_info_t *next_image_info;

	if (!bl33_ep_info.h.version) {
		SET_PARAM_HEAD(&bl33_ep_info, PARAM_EP, VERSION_1, 0);
		SET_SECURITY_STATE(bl33_ep_info.h.attr, NON_SECURE);
		bl33_ep_info.pc = BL33_BASE;
		bl33_ep_info.spsr = mtk_default_spsr_for_bl33_entry(false, true);
	}

	if (!bl32_ep_info.h.version) {
		SET_PARAM_HEAD(&bl32_ep_info, PARAM_EP, VERSION_1, 0);
		SET_SECURITY_STATE(bl32_ep_info.h.attr, SECURE);
		bl32_ep_info.pc = BL32_BASE;
		bl32_ep_info.spsr = mtk_default_spsr_for_bl32_entry();
	}

	/* Populate entry point information for BL3-2 and BL3-3 */
	if (type == NON_SECURE)
		next_image_info = &bl33_ep_info;
	else
		next_image_info = &bl32_ep_info;

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;

	return NULL;
}
