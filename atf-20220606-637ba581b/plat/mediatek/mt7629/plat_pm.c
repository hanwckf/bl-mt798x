/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <errno.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <drivers/arm/gic_common.h>
#include <drivers/arm/gicv2.h>
#include <lib/mmio.h>
#include <lib/psci/psci.h>
#include <plat/common/platform.h>
#include <plat_private.h>

#include <platform_def.h>
#include <spmc.h>
#include <mcucfg.h>

/*
 * Macros for local power states in MTK platforms encoded by State-ID field
 * within the power-state parameter.
 */

/* Local power state for power domains in Run state. */
#define MTK_LOCAL_STATE_RUN		0
/* Local power state for retention. Valid only for CPU power domains */
#define MTK_LOCAL_STATE_RET		1
/* Local power state for OFF/power-down. Valid for CPU and cluster power
 * domains
 */
#define MTK_LOCAL_STATE_OFF		2


static uintptr_t mt_sec_entrypoint;
static uint32_t cntfrq_core0;

static void mt_cpu_standby(plat_local_state_t cpu_state)
{
	uint32_t interrupt = GIC_SPURIOUS_INTERRUPT;

	assert(cpu_state == ARM_LOCAL_STATE_RET);

	/*
	 * Enter standby state
	 * dsb is good practice before using wfi to enter low power states
	 */
	isb();
	dsb();
	while (interrupt == GIC_SPURIOUS_INTERRUPT) {
		wfi();

		/* Acknoledge IT */
		interrupt = gicv2_acknowledge_interrupt();
		/* If Interrupt == 1022 it will be acknowledged by non secure */
		if ((interrupt != PENDING_G1_INTID) &&
		    (interrupt != GIC_SPURIOUS_INTERRUPT)) {
			gicv2_end_of_interrupt(interrupt);
		}
	}
}

static int mt_pwr_domain_on(u_register_t mpidr)
{
	int rc = PSCI_E_SUCCESS;
	unsigned long cpu_id;

	cpu_id = mpidr & MPIDR_CPU_MASK;

	/* enable bootrom power down mode */
	mmio_setbits_32(SRAMROM_SEC_CTRL, SW_ROM_PD);

	/* FIXME: is it MBOX address (boot address)? */
	mmio_write_32(SRAMROM_BOOT_ADDR, mt_sec_entrypoint);
	mmio_write_32(BOOT_META0, mt_sec_entrypoint);

	INFO("cpu_on[%ld], entry %x\n",
	     cpu_id, mmio_read_32(SRAMROM_BOOT_ADDR));

	mtk_spm_cpu1_power_on();

	return rc;
}

static void
mt_pwr_domain_off(const psci_power_state_t *target_state)
{
	mtk_spm_cpu1_power_off();
}


static void
mt_pwr_domain_suspend(const psci_power_state_t *target_state)
{
	/* Nothing to do, power domain is not disabled */
}

static void
mt_pwr_domain_on_finish(const psci_power_state_t *target_state)
{

	int idx;

	assert(target_state->pwr_domain_state[MPIDR_AFFLVL0] ==
	       MTK_LOCAL_STATE_OFF);

	write_cntfrq_el0(cntfrq_core0);

	/* enable GiC per-CPU enable registers */
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();

	/* set pol control as non-secure */
	for (idx = 0; idx < INT_POL_SECCTL_NUM; idx++)
		mmio_write_32(INT_POL_SECCTL0 + idx * 4, 0);
}


static void
mt_pwr_domain_suspend_finish(const psci_power_state_t *target_state)
{
	/* Nothing to do, power domain is not disabled */
}

static void __dead2
mt_pwr_domain_pwr_down_wfi(const psci_power_state_t *target_state)
{
	ERROR("mtmpu1 Power Down WFI: operation not handled.\n");
	panic();
}

static void __dead2 mt_system_off(void)
{
	ERROR("mtmpu1 System Off: operation not handled.\n");
	panic();
}

static void __dead2 mt_system_reset(void)
{
	/* Write the System Configuration Control Register */
	INFO("MTK System Reset\n");

	mmio_clrsetbits_32(MTK_WDT_BASE,
			   (MTK_WDT_MODE_DUAL_MODE | MTK_WDT_MODE_IRQ),
			   MTK_WDT_MODE_KEY);
	mmio_setbits_32(MTK_WDT_BASE, (MTK_WDT_MODE_KEY | MTK_WDT_MODE_EXTEN));
	mmio_setbits_32(MTK_WDT_SWRST, MTK_WDT_SWRST_KEY);

	wfi();
	ERROR("MTK System Reset: operation not handled.\n");
	panic();
}

static int mt_validate_power_state(unsigned int power_state,
				   psci_power_state_t *req_state)
{
	int pstate = psci_get_pstate_type(power_state);

	if (pstate != 0)
		return PSCI_E_INVALID_PARAMS;

	if (psci_get_pstate_pwrlvl(power_state))
		return PSCI_E_INVALID_PARAMS;

	if (psci_get_pstate_id(power_state))
		return PSCI_E_INVALID_PARAMS;

	req_state->pwr_domain_state[0] = U(1);
	req_state->pwr_domain_state[1] = U(0);

	return PSCI_E_SUCCESS;
}

static int mt_validate_ns_entrypoint(uintptr_t entrypoint)
{
	return PSCI_E_SUCCESS;
}

static int
mt_node_hw_state(u_register_t target_cpu, unsigned int power_level)
{
	/*
	 * The format of 'power_level' is implementation-defined, but 0 must
	 * mean a CPU. Only allow level 0.
	 */
	if (power_level != MPIDR_AFFLVL0)
		return PSCI_E_INVALID_PARAMS;


	/*
	 * From psci view the CPU 0 is always ON,
	 * CPU 1 can be SUSPEND or RUNNING.
	 * Therefore do not manage POWER OFF state and always return HW_ON.
	 */

	return (int)HW_ON;
}

/*******************************************************************************
 * Export the platform handlers. The ARM Standard platform layer will take care
 * of registering the handlers with PSCI.
 ******************************************************************************/
static const plat_psci_ops_t mt_psci_ops = {
	.cpu_standby = mt_cpu_standby,
	.pwr_domain_on = mt_pwr_domain_on,
	.pwr_domain_off = mt_pwr_domain_off,
	.pwr_domain_suspend = mt_pwr_domain_suspend,
	.pwr_domain_on_finish = mt_pwr_domain_on_finish,
	.pwr_domain_suspend_finish = mt_pwr_domain_suspend_finish,
	.pwr_domain_pwr_down_wfi = mt_pwr_domain_pwr_down_wfi,
	.system_off = mt_system_off,
	.system_reset = mt_system_reset,
	.validate_power_state = mt_validate_power_state,
	.validate_ns_entrypoint = mt_validate_ns_entrypoint,
	.get_node_hw_state = mt_node_hw_state
};

/*******************************************************************************
 * Export the platform specific power ops.
 ******************************************************************************/
int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	mt_sec_entrypoint = sec_entrypoint;
	*psci_ops = &mt_psci_ops;

	return 0;
}
