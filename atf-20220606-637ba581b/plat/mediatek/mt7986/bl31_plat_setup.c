/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.h>
#include <lib/el3_runtime/context_mgmt.h>
#include <common/debug.h>
#include <drivers/generic_delay_timer.h>
#include <drivers/ti/uart/uart_16550.h>
#include <lib/mmio.h>
#include <plat_arm.h>
#include <plat_private.h>
#include <plat/common/common_def.h>
#include <plat/common/platform.h>
#include <timer.h>
#include <drivers/delay_timer.h>

#include <mcucfg.h>
#include <mtk_plat_common.h>
#include <mtspmc.h>
#include <mt_gic_v3.h>
#include <assert.h>
#include <hsuart.h>
#include <emi_mpu.h>
#include <devapc.h>
#include <efuse_cmd.h>

static entry_point_info_t bl32_ep_info;
static entry_point_info_t bl33_ep_info;

void mt7986_disable_l2c_shared(void)
{
	uint32_t inval_l2_tags_complete;

	dsb();
	isb();
	/* Flush and invalidate data cache */
	dcsw_op_all(DCCISW);
	dsb();
	isb();

	mmio_setbits_32((uintptr_t)&mt7986_mcucfg->mp0_misc_config3, BIT(29));
	do {
		inval_l2_tags_complete = mmio_read_32(
			(uintptr_t)&mt7986_mcucfg->mp0_misc_config3);
		inval_l2_tags_complete &= BIT(30);

	} while (!inval_l2_tags_complete);
	mmio_clrbits_32((uintptr_t)&mt7986_mcucfg->mp0_misc_config3, BIT(29));
	mmio_write_32((uintptr_t)&mt7986_mcucfg->l2c_cfg_mp0, 0x300);
}

void platform_setup_cpu(void)
{
	/* change shared sram back to L2C */
	uint32_t l2c_cfg_mp0 = mmio_read_32(
			(uintptr_t)&mt7986_mcucfg->l2c_cfg_mp0);
	uint32_t mp0_l2c_size_cfg = ((l2c_cfg_mp0 & 0xf00) >> 8);

	INFO("Total CPU count: %d\n", PLATFORM_CORE_COUNT);

	if (l2c_cfg_mp0 & BIT(0)) {
		switch (mp0_l2c_size_cfg) {
		case 0x3:
			INFO("MCUSYS: Disable 512KB L2C shared SRAM\n");
			mt7986_disable_l2c_shared();
			break;
		case 0x1:
			INFO("MCUSYS: Disable 256KB L2C shared SRAM\n");
			mt7986_disable_l2c_shared();
			break;
		default:
			break;
		}
	}
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

	next_image_info = (type == NON_SECURE) ? &bl33_ep_info : &bl32_ep_info;

	/* None of the images on this platform can have 0x0 as the entrypoint */
	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

typedef enum KERNEL_ARCH {
	KERNEL_AARCH32,
	KERNEL_AARCH64

} kernel_arch;

entry_point_info_t *bl31_plat_get_next_kernel_ep_info(kernel_arch k32_64)
{
	entry_point_info_t *next_image_info;

	next_image_info = &bl33_ep_info;

	if (k32_64 == KERNEL_AARCH32) {
		INFO("Kernel is AARCH32\n");
		next_image_info->spsr = SPSR_MODE32(NON_SECURE, SPSR_T_ARM,
						SPSR_E_LITTLE, (DAIF_FIQ_BIT |
						DAIF_IRQ_BIT | DAIF_ABT_BIT));
	} else if (k32_64 == KERNEL_AARCH64) {
		INFO("Kernel is AARCH64\n");
		next_image_info->spsr = SPSR_64(NON_SECURE, MODE_SP_ELX,
						DISABLE_ALL_EXCEPTIONS);
	} else {
		return NULL;
	}

	next_image_info->pc = get_kernel_info_pc();
	next_image_info->args.arg0 = get_kernel_info_r0();
	next_image_info->args.arg1 = get_kernel_info_r1();
	next_image_info->args.arg2 = get_kernel_info_r2();

	INFO("pc=0x%llx, r0=0x%llx, r1=0x%llx, r2=0x%llx\n",
		(unsigned long long)next_image_info->pc,
		(unsigned long long)next_image_info->args.arg0,
		(unsigned long long)next_image_info->args.arg1,
		(unsigned long long)next_image_info->args.arg2);

	SET_SECURITY_STATE(next_image_info->h.attr, NON_SECURE);

	if (next_image_info->pc)
		return next_image_info;
	else
		return NULL;
}

void bl31_prepare_kernel_entry(uint64_t k32_64)
{
	entry_point_info_t *next_image_info;
	uint32_t image_type;

	/* Determine which image to execute next */
	/* image_type = bl31_get_next_image_type(); */
	image_type = NON_SECURE;

	/* Program EL3 registers to enable entry into the next EL */
	if (k32_64 == 0)
		next_image_info = bl31_plat_get_next_kernel_ep_info(KERNEL_AARCH32);
	else
		next_image_info = bl31_plat_get_next_kernel_ep_info(KERNEL_AARCH64);

	assert(next_image_info);
	assert(image_type == GET_SECURITY_STATE(next_image_info->h.attr));

	INFO("BL3-1: Preparing for EL3 exit to %s world, Kernel\n",
		(image_type == SECURE) ? "secure" : "normal");
	INFO("BL3-1: Next image address = 0x%llx\n",
		(unsigned long long) next_image_info->pc);
	INFO("BL3-1: Next image spsr = 0x%x\n", next_image_info->spsr);
	cm_init_my_context(next_image_info);
	cm_prepare_el3_exit(image_type);
}

uint32_t plat_get_spsr_for_bl32_entry(void)
{
	/*
	 * The Secure Payload Dispatcher service is responsible for
	 * setting the SPSR prior to entry into the BL3-2 image.
	 */
	return 0;
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
	int check_ver;

	console_hsuart_register(UART_BASE, UART_CLOCK, UART_BAUDRATE,
			        true, &console);

	platform_setup_cpu();

	check_ver = check_version();
	INFO("check_ver = %d\n", check_ver);

	/* Populate entry point information for BL3-2 and BL3-3 */
	SET_PARAM_HEAD(&bl32_ep_info, PARAM_EP, VERSION_1, 0);
	SET_SECURITY_STATE(bl32_ep_info.h.attr, SECURE);
	bl32_ep_info.pc = BL32_BASE;
	bl32_ep_info.spsr = plat_get_spsr_for_bl32_entry();

	SET_PARAM_HEAD(&bl33_ep_info, PARAM_EP, VERSION_1, 0);
	/*
	 * Tell BL3-1 where the non-trusted software image
	 * is located and the entry state information
	 */
	bl33_ep_info.pc = BL33_BASE;
	bl33_ep_info.spsr = plat_get_spsr_for_bl33_entry();
	SET_SECURITY_STATE(bl33_ep_info.h.attr, NON_SECURE);
}

/*******************************************************************************
 * Perform any BL3-1 platform setup code
 ******************************************************************************/
void bl31_platform_setup(void)
{
	mtk_timer_init();

	emi_mpu_init();

	devapc_init();
}

/*******************************************************************************
 * Perform the very early platform specific architectural setup here. At the
 * moment this is only intializes the mmu in a quick and dirty way.
 ******************************************************************************/
void bl31_plat_arch_setup(void)
{
#if 0
	plat_cci_init();
	plat_cci_enable();
#endif

	/* FIXME: unknown issue
	 * Add GIC regine to TTBR will damage MMU during
	 * psci SMC call "smc 0x0C4000003 0x1"
	 * don't know this is software issue, CPU design issue,
	 * data cache issue, or MMU issue,
	 * but this will cause ATF panic.
	 * I have no idea about this issue currentlly,
	 * just use a software solution that init gic before mmu enable
	 */
	/* Initialize the gic cpu and distributor interfaces */
	plat_mt_gic_init();
	spmc_init();
	/* enable mmu */
	plat_configure_mmu_el3(BL_CODE_BASE,
			       BL_COHERENT_RAM_END - BL_CODE_BASE,
			       BL_CODE_BASE,
			       BL_CODE_END,
			       BL_COHERENT_RAM_BASE,
			       BL_COHERENT_RAM_END);
}
