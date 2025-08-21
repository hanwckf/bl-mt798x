/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <lib/psci/psci.h>
#include <lib/mmio.h>
#include <mtspmc.h>
#include <mcucfg.h>
#include <plat_pm.h>
#include <cpuxgpt.h>

uintptr_t secure_entrypoint;

int mtk_plat_power_domain_on(unsigned long mpidr)
{
	uintptr_t rv = (uintptr_t)&mt7988_mcucfg->cpucfg;
	unsigned long cpu_id = mpidr & MPIDR_CPU_MASK;
	int rc = PSCI_E_SUCCESS;

	INFO("%s: cpu: %ld (total cpu: %d), rv=0x%lx\n", __func__, cpu_id,
	     PLATFORM_CORE_COUNT, rv);

	mmio_write_32(rv, MP0_CPUCFG_64BIT);

	switch (cpu_id) {
	case 0:
		rv = (uintptr_t)&mt7988_mcucfg->rvaddr0_l;
		break;
	case 1:
		rv = (uintptr_t)&mt7988_mcucfg->rvaddr1_l;
		break;
	case 2:
		rv = (uintptr_t)&mt7988_mcucfg->rvaddr2_l;
		break;
	case 3:
		rv = (uintptr_t)&mt7988_mcucfg->rvaddr3_l;
		break;
	default:
		return PSCI_E_NOT_PRESENT;
	}

	mmio_write_32(rv, secure_entrypoint);

	INFO("cpu_on[%ld], entry %x\n", cpu_id, mmio_read_32(rv));

	spmc_cpu_corex_onoff(cpu_id, STA_POWER_ON, MODE_SPMC_HW);

	return rc;
}

void mtk_plat_power_domain_on_finish(const psci_power_state_t *state)
{
	// unsigned long mpidr = read_mpidr_el1();

	assert(state->pwr_domain_state[MPIDR_AFFLVL0] == MTK_LOCAL_STATE_OFF);

	if (PLAT_MAX_PWR_LVL > MTK_PWR_LVL1 &&
	    state->pwr_domain_state[MTK_PWR_LVL2] == MTK_LOCAL_STATE_OFF)
		mtk_system_pwr_domain_resume();

	if (state->pwr_domain_state[MPIDR_AFFLVL1] == MTK_LOCAL_STATE_OFF) {
		// plat_cci_enable();
		// trace_power_flow(mpidr, CLUSTER_UP);
	}

	if (PLAT_MAX_PWR_LVL > MTK_PWR_LVL1 &&
	    state->pwr_domain_state[MTK_PWR_LVL2] == MTK_LOCAL_STATE_OFF)
		return;

	// trace_power_flow(mpidr, CPU_UP);
}

void mtk_plat_power_domain_off(const psci_power_state_t *state)
{
	unsigned long mpidr = read_mpidr_el1();
	unsigned long cpu_id = mpidr & MPIDR_CPU_MASK;

	// trace_power_flow(mpidr, CPU_DOWN);

	if (MTK_CLUSTER_PWR_STATE(state) == MTK_LOCAL_STATE_OFF) {
		/* Disable coherency if this cluster is to be turned off */
		// plat_cci_disable();
		// trace_power_flow(mpidr, CLUSTER_DOWN);
	}

	INFO("cpu_off[%ld]\n", cpu_id);

	spmc_cpu_corex_onoff(cpu_id, STA_POWER_DOWN, MODE_AUTO_SHUT_OFF);
}

void mtk_plat_power_domain_suspend(const psci_power_state_t *state)
{
	unsigned long mpidr = read_mpidr_el1();
	// unsigned long cpu_id;
	// uintptr_t rv;

	// cpu_id = mpidr & MPIDR_CPU_MASK;

	/* FIXME: is it MBOX address (boot address)? */
	// rv = (uintptr_t)&mt7622_mcucfg->mp0_misc_config[(cpu_id + 1) * 2];
	// mmio_write_32(rv, secure_entrypoint);

	if (MTK_SYSTEM_PWR_STATE(state) != MTK_LOCAL_STATE_OFF) {
		// spm_mcdi_prepare_for_off_state(mpidr, MTK_PWR_LVL0);
		// if (MTK_CLUSTER_PWR_STATE(state) == MTK_LOCAL_STATE_OFF)
		//	spm_mcdi_prepare_for_off_state(mpidr, MTK_PWR_LVL1);
	}

	mt_platform_save_context(mpidr);

	/* Perform the common cluster specific operations */
	if (MTK_CLUSTER_PWR_STATE(state) == MTK_LOCAL_STATE_OFF) {
		/* Disable coherency if this cluster is to be turned off */
		// plat_cci_disable();
	}

	if (MTK_SYSTEM_PWR_STATE(state) == MTK_LOCAL_STATE_OFF) {
		// disable_scu(mpidr);
		generic_timer_backup();
		// spm_system_suspend();
	}
}

void mtk_plat_power_domain_suspend_finish(const psci_power_state_t *state)
{
	unsigned long mpidr = read_mpidr_el1();

	if (state->pwr_domain_state[MTK_PWR_LVL0] == MTK_LOCAL_STATE_RET)
		return;

	if (MTK_SYSTEM_PWR_STATE(state) == MTK_LOCAL_STATE_OFF) {
		// spm_system_suspend_finish();
		// enable_scu(mpidr);
	}

	/* Perform the common cluster specific operations */
	if (MTK_CLUSTER_PWR_STATE(state) == MTK_LOCAL_STATE_OFF) {
		/* Enable coherency if this cluster was off */
		// plat_cci_enable();
	}

	mt_platform_restore_context(mpidr);

	if (MTK_SYSTEM_PWR_STATE(state) != MTK_LOCAL_STATE_OFF) {
		// spm_mcdi_finish_for_on_state(mpidr, MTK_PWR_LVL0);
		// if (MTK_CLUSTER_PWR_STATE(state) == MTK_LOCAL_STATE_OFF)
		//	spm_mcdi_finish_for_on_state(mpidr, MTK_PWR_LVL1);
	}
}
