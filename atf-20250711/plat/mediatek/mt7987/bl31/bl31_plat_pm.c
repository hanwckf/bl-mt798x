// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#include <assert.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <lib/psci/psci.h>
#include <lib/mmio.h>
#include <mtk_gic_v3.h>
#include <mtspmc.h>
#include <mcucfg.h>
#include <plat_pm.h>

uintptr_t secure_entrypoint;

int mtk_plat_power_domain_on(unsigned long mpidr)
{
	int cpu_id = MPIDR_AFFLVL0_VAL(mpidr);
	uint32_t bootaddr_h, bootaddr_l;
	int arm64 = 1;
	int map;

	map = mmio_read_32(0x11d30040) & GENMASK(4, 0);
	if (cpu_id >= PLATFORM_CORE_COUNT)
		return PSCI_E_NOT_SUPPORTED;

	if ((map >> cpu_id) & BIT(0))
		return PSCI_E_NOT_SUPPORTED;

	bootaddr_h = (secure_entrypoint >> 32) & MCUCFG_BOOTADDR_H_MASK;
	bootaddr_l = secure_entrypoint & MCUCFG_BOOTADDR_L_MASK;

	mmio_write_32(mcucfg_reg(MCUCFG_BOOTADDR_H[cpu_id]), bootaddr_h);
	mmio_write_32(mcucfg_reg(MCUCFG_BOOTADDR_L[cpu_id]), bootaddr_l);

	/* In some case MCUCFG_BOOTADDR_H[cpu_id] == mcucfg_reg(MCUCFG_INITARCH)
	 * So we should set MCUCFG_INITARCH after MCUCFG_BOOTADDR_H
	 */
	if (arm64)
		mmio_setbits_32(mcucfg_reg(MCUCFG_INITARCH),
				1 << (MCUCFG_INITARCH_SHIFT + cpu_id));
	else
		mmio_clrbits_32(mcucfg_reg(MCUCFG_INITARCH),
				1 << (MCUCFG_INITARCH_SHIFT + cpu_id));

	INFO("cpu_on[%d], entrypoint 0x%zx\n", cpu_id, secure_entrypoint);
	spmc_cpu_corex_onoff(cpu_id, STA_POWER_ON, MODE_SPMC_HW);

	return PSCI_E_SUCCESS;
}

void mtk_plat_power_domain_off(const psci_power_state_t *state)
{
	uint64_t mpidr = read_mpidr();
	int cpu_id = MPIDR_AFFLVL0_VAL(mpidr);

	INFO("cpu_off[%d]\n", cpu_id);
	spmc_cpu_corex_onoff(cpu_id, STA_POWER_DOWN, MODE_AUTO_SHUT_OFF);
}
