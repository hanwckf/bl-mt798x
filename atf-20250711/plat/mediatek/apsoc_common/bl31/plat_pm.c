// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <assert.h>
#include <errno.h>
#include <cdefs.h>

#include <arch_helpers.h>
#include <common/debug.h>
#include <lib/bakery_lock.h>
#include <lib/psci/psci.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <plat_arm.h>
#include <plat_private.h>
#include <hsuart.h>
#include <mtk_wdt.h>
#include "plat_pm.h"

/*
 * Macros for local power states in MTK platforms encoded by State-ID field
 * within the power-state parameter.
 */

#if PSCI_EXTENDED_STATE_ID
/*
 * Macros used to parse state information from State-ID if it is using the
 * recommended encoding for State-ID.
 */
#define MTK_LOCAL_PSTATE_WIDTH		4
#define MTK_LOCAL_PSTATE_MASK		((1 << MTK_LOCAL_PSTATE_WIDTH) - 1)

/* Macros to construct the composite power state */

/* Make composite power state parameter till power level 0 */

#define mtk_make_pwrstate_lvl0(lvl0_st, pwr_lvl, type)		\
	(((lvl0_st) << PSTATE_ID_SHIFT) |			\
	 ((type) << PSTATE_TYPE_SHIFT))
#else
#define mtk_make_pwrstate_lvl0(lvl0_st, pwr_lvl, type)		\
	(((lvl0_st) << PSTATE_ID_SHIFT) |			\
	 ((pwr_lvl) << PSTATE_PWR_LVL_SHIFT) |			\
	 ((type) << PSTATE_TYPE_SHIFT))

#endif /* __PSCI_EXTENDED_STATE_ID__ */

/* Make composite power state parameter till power level 1 */
#define mtk_make_pwrstate_lvl1(lvl1_st, lvl0_st, pwr_lvl, type)	\
	(((lvl1_st) << MTK_LOCAL_PSTATE_WIDTH) |		\
	 mtk_make_pwrstate_lvl0(lvl0_st, pwr_lvl, type))

/* Make composite power state parameter till power level 2 */
#define mtk_make_pwrstate_lvl2(lvl2_st, lvl1_st, lvl0_st, pwr_lvl, type) \
	(((lvl2_st) << (MTK_LOCAL_PSTATE_WIDTH * 2)) |			 \
	 mtk_make_pwrstate_lvl1(lvl1_st, lvl0_st, pwr_lvl, type))

#if PSCI_EXTENDED_STATE_ID
/*
 *  The table storing the valid idle power states. Ensure that the
 *  array entries are populated in ascending order of state-id to
 *  enable us to use binary search during power state validation.
 *  The table must be terminated by a NULL entry.
 */
const unsigned int mtk_pm_idle_states[] = {
	/* State-id - 0x001 */
	mtk_make_pwrstate_lvl2(MTK_LOCAL_STATE_RUN, MTK_LOCAL_STATE_RUN,
			       MTK_LOCAL_STATE_RET, MTK_PWR_LVL0,
			       PSTATE_TYPE_STANDBY),
	/* State-id - 0x002 */
	mtk_make_pwrstate_lvl2(MTK_LOCAL_STATE_RUN, MTK_LOCAL_STATE_RUN,
			       MTK_LOCAL_STATE_OFF, MTK_PWR_LVL0,
			       PSTATE_TYPE_POWERDOWN),
	/* State-id - 0x022 */
	mtk_make_pwrstate_lvl2(MTK_LOCAL_STATE_RUN, MTK_LOCAL_STATE_OFF,
			       MTK_LOCAL_STATE_OFF, MTK_PWR_LVL1,
			       PSTATE_TYPE_POWERDOWN),
#if PLAT_MAX_PWR_LVL > MTK_PWR_LVL1
	/* State-id - 0x222 */
	mtk_make_pwrstate_lvl2(MTK_LOCAL_STATE_OFF, MTK_LOCAL_STATE_OFF,
			       MTK_LOCAL_STATE_OFF, MTK_PWR_LVL2,
			       PSTATE_TYPE_POWERDOWN),
#endif
	0,
};
#endif

struct core_context {
	unsigned long timer_data[8];
	unsigned int count;
	unsigned int rst;
	unsigned int abt;
	unsigned int brk;
};

struct cluster_context {
	struct core_context core[PLATFORM_MAX_CPUS_PER_CLUSTER];
};

/*
 * Top level structure to hold the complete context of a multi cluster system
 */
struct system_context {
	struct cluster_context cluster[PLATFORM_CLUSTER_COUNT];
};

/*
 * Top level structure which encapsulates the context of the entire system
 */
static struct system_context dormant_data[1];

static struct cluster_context *system_cluster(struct system_context *system,
					      uint32_t clusterid)
{
	return &system->cluster[clusterid];
}

static struct core_context *cluster_core(struct cluster_context *cluster,
					 uint32_t cpuid)
{
	return &cluster->core[cpuid];
}

static struct cluster_context *get_cluster_data(unsigned long mpidr)
{
	uint32_t clusterid;

	clusterid = (mpidr & MPIDR_CLUSTER_MASK) >> MPIDR_AFFINITY_BITS;

	return system_cluster(dormant_data, clusterid);
}

static struct core_context *get_core_data(unsigned long mpidr)
{
	struct cluster_context *cluster;
	uint32_t cpuid;

	cluster = get_cluster_data(mpidr);
	cpuid = mpidr & MPIDR_CPU_MASK;

	return cluster_core(cluster, cpuid);
}

static void mt_save_generic_timer(unsigned long *container)
{
	uint64_t ctl;
	uint64_t val;

	__asm__ volatile("mrs	%x0, cntkctl_el1\n\t"
			 "mrs	%x1, cntp_cval_el0\n\t"
			 "stp	%x0, %x1, [%2, #0]"
			 : "=&r"(ctl), "=&r"(val)
			 : "r"(container)
			 : "memory");

	__asm__ volatile("mrs	%x0, cntp_tval_el0\n\t"
			 "mrs	%x1, cntp_ctl_el0\n\t"
			 "stp	%x0, %x1, [%2, #16]"
			 : "=&r"(val), "=&r"(ctl)
			 : "r"(container)
			 : "memory");

	__asm__ volatile("mrs	%x0, cntv_tval_el0\n\t"
			 "mrs	%x1, cntv_ctl_el0\n\t"
			 "stp	%x0, %x1, [%2, #32]"
			 : "=&r"(val), "=&r"(ctl)
			 : "r"(container)
			 : "memory");
}

static void mt_restore_generic_timer(unsigned long *container)
{
	uint64_t ctl;
	uint64_t val;

	__asm__ volatile("ldp	%x0, %x1, [%2, #0]\n\t"
			 "msr	cntkctl_el1, %x0\n\t"
			 "msr	cntp_cval_el0, %x1"
			 : "=&r"(ctl), "=&r"(val)
			 : "r"(container)
			 : "memory");

	__asm__ volatile("ldp	%x0, %x1, [%2, #16]\n\t"
			 "msr	cntp_tval_el0, %x0\n\t"
			 "msr	cntp_ctl_el0, %x1"
			 : "=&r"(val), "=&r"(ctl)
			 : "r"(container)
			 : "memory");

	__asm__ volatile("ldp	%x0, %x1, [%2, #32]\n\t"
			 "msr	cntv_tval_el0, %x0\n\t"
			 "msr	cntv_ctl_el0, %x1"
			 : "=&r"(val), "=&r"(ctl)
			 : "r"(container)
			 : "memory");
}

static inline uint64_t read_cntpctl(void)
{
	uint64_t cntpctl;

	__asm__ volatile("mrs	%x0, cntp_ctl_el0"
			 : "=r"(cntpctl)
			 :
			 : "memory");

	return cntpctl;
}

static inline void write_cntpctl(uint64_t cntpctl)
{
	__asm__ volatile("msr	cntp_ctl_el0, %x0" : : "r"(cntpctl));
}

static void stop_generic_timer(void)
{
	/*
	 * Disable the timer and mask the irq to prevent
	 * suprious interrupts on this cpu interface. It
	 * will bite us when we come back if we don't. It
	 * will be replayed on the inbound cluster.
	 */
	uint64_t cntpctl = read_cntpctl();

	write_cntpctl(clr_cntp_ctl_enable(cntpctl));
}

static void mt_cpu_save(unsigned long mpidr)
{
	struct core_context *core;

	core = get_core_data(mpidr);
	mt_save_generic_timer(core->timer_data);

	/* disable timer irq, and upper layer should enable it again. */
	stop_generic_timer();
}

static void mt_cpu_restore(unsigned long mpidr)
{
	struct core_context *core;

	core = get_core_data(mpidr);
	mt_restore_generic_timer(core->timer_data);
}

void mt_platform_save_context(unsigned long mpidr)
{
	/* mcusys_save_context: */
	mt_cpu_save(mpidr);
}

void mt_platform_restore_context(unsigned long mpidr)
{
	/* mcusys_restore_context: */
	mt_cpu_restore(mpidr);
}

static void plat_cpu_standby(plat_local_state_t cpu_state)
{
	unsigned int scr;

	scr = read_scr_el3();
	write_scr_el3(scr | SCR_IRQ_BIT);
	isb();
	dsb();
	wfi();
	write_scr_el3(scr);
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be turned
 * on. The level and mpidr determine the affinity instance.
 ******************************************************************************/
#pragma weak mtk_plat_power_domain_on
int mtk_plat_power_domain_on(unsigned long mpidr)
{
	return PSCI_E_NOT_SUPPORTED;
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be turned
 * off. The level and mpidr determine the affinity instance. The 'state' arg.
 * allows the platform to decide whether the cluster is being turned off and
 * take apt actions.
 *
 * CAUTION: This function is called with coherent stacks so that caches can be
 * turned off, flushed and coherency disabled. There is no guarantee that caches
 * will remain turned on across calls to this function as each affinity level is
 * dealt with. So do not write & read global variables across calls. It will be
 * wise to do flush a write to the global to prevent unpredictable results.
 ******************************************************************************/
#pragma weak mtk_plat_power_domain_off
void mtk_plat_power_domain_off(const psci_power_state_t *state)
{
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance is about to be
 * suspended. The level and mpidr determine the affinity instance. The 'state'
 * arg. allows the platform to decide whether the cluster is being turned off
 * and take apt actions.
 *
 * CAUTION: This function is called with coherent stacks so that caches can be
 * turned off, flushed and coherency disabled. There is no guarantee that caches
 * will remain turned on across calls to this function as each affinity level is
 * dealt with. So do not write & read global variables across calls. It will be
 * wise to do flush a write to the global to prevent unpredictable results.
 ******************************************************************************/
#pragma weak mtk_plat_power_domain_suspend
void mtk_plat_power_domain_suspend(const psci_power_state_t *state)
{
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance has just been powered
 * on after being turned off earlier. The level and mpidr determine the affinity
 * instance. The 'state' arg. allows the platform to decide whether the cluster
 * was turned off prior to wakeup and do what's necessary to setup it up
 * correctly.
 ******************************************************************************/
#pragma weak mtk_plat_power_domain_on_finish
void mtk_plat_power_domain_on_finish(const psci_power_state_t *state)
{
}

/*******************************************************************************
 * MTK_platform handler called when an affinity instance has just been powered
 * on after having been suspended earlier. The level and mpidr determine the
 * affinity instance.
 ******************************************************************************/
#pragma weak mtk_plat_power_domain_suspend_finish
void mtk_plat_power_domain_suspend_finish(const psci_power_state_t *state)
{
}

static void plat_get_sys_suspend_power_state(psci_power_state_t *req_state)
{
	assert(PLAT_MAX_PWR_LVL >= 2);

	for (int i = MPIDR_AFFLVL0; i <= PLAT_MAX_PWR_LVL; i++)
		req_state->pwr_domain_state[i] = MTK_LOCAL_STATE_OFF;
}

/*******************************************************************************
 * MTK handlers to shutdown/reboot the system
 ******************************************************************************/
#pragma weak mtk_plat_system_reset
static void __dead2 mtk_plat_system_reset(void)
{
	INFO("MTK System Reset\n");

	mtk_wdt_soft_reset();

	wfi();
	ERROR("MTK System Reset: operation not handled.\n");
	panic();
}

#pragma weak mtk_plat_system_off
static void __dead2 mtk_plat_system_off(void)
{
	console_switch_state(CONSOLE_FLAG_BOOT);
	ERROR("Power-down unsupported.\n");
	console_switch_state(CONSOLE_FLAG_RUNTIME);

	mtk_plat_system_reset();
}

#pragma weak mtk_plat_system_reset2
static int __dead2 mtk_plat_system_reset2(int is_vendor, int reset_type,
					  u_register_t cookie)
{
	INFO("MTK System Exception Reset\n");

	mmio_setbits_32(MTK_DDR_RESERVE_MODE,
			(MTK_DDR_RESERVE_ENABLE | MTK_DDR_RESERVE_KEY));

	mtk_wdt_soft_reset();

	wfi();
	ERROR("MTK System Reset: operation not handled.\n");
	panic();
}

#if !PSCI_EXTENDED_STATE_ID
static int plat_validate_power_state(unsigned int power_state,
				     psci_power_state_t *req_state)
{
	int pstate = psci_get_pstate_type(power_state);
	int pwr_lvl = psci_get_pstate_pwrlvl(power_state);
	int i;

	assert(req_state);

	if (pwr_lvl > PLAT_MAX_PWR_LVL)
		return PSCI_E_INVALID_PARAMS;

	/* Sanity check the requested state */
	if (pstate == PSTATE_TYPE_STANDBY) {
		/*
		 * It's possible to enter standby only on power level 0
		 * Ignore any other power level.
		 */
		if (pwr_lvl != 0)
			return PSCI_E_INVALID_PARAMS;

		req_state->pwr_domain_state[MTK_PWR_LVL0] = MTK_LOCAL_STATE_RET;
	} else {
		for (i = 0; i <= pwr_lvl; i++)
			req_state->pwr_domain_state[i] = MTK_LOCAL_STATE_OFF;
	}

	/*
	 * We expect the 'state id' to be zero.
	 */
	if (psci_get_pstate_id(power_state))
		return PSCI_E_INVALID_PARAMS;

	return PSCI_E_SUCCESS;
}
#else
int plat_validate_power_state(unsigned int power_state,
			      psci_power_state_t *req_state)
{
	unsigned int state_id;
	int i;

	assert(req_state);

	/*
	 *  Currently we are using a linear search for finding the matching
	 *  entry in the idle power state array. This can be made a binary
	 *  search if the number of entries justify the additional complexity.
	 */
	for (i = 0; !!mtk_pm_idle_states[i]; i++) {
		if (power_state == mtk_pm_idle_states[i])
			break;
	}

	/* Return error if entry not found in the idle state array */
	if (!mtk_pm_idle_states[i])
		return PSCI_E_INVALID_PARAMS;

	i = 0;
	state_id = psci_get_pstate_id(power_state);

	/* Parse the State ID and populate the state info parameter */
	while (state_id) {
		req_state->pwr_domain_state[i++] =
			state_id & MTK_LOCAL_PSTATE_MASK;
		state_id >>= MTK_LOCAL_PSTATE_WIDTH;
	}

	return PSCI_E_SUCCESS;
}
#endif

void mtk_system_pwr_domain_resume(void)
{
	static console_t console;

	console_hsuart_register(UART_BASE, UART_CLOCK, UART_BAUDRATE, true,
				&console);

	/* Assert system power domain is available on the platform */
	assert(PLAT_MAX_PWR_LVL >= MTK_PWR_LVL2);
}

static const plat_psci_ops_t plat_plat_pm_ops = {
	.cpu_standby			= plat_cpu_standby,
	.pwr_domain_on			= mtk_plat_power_domain_on,
	.pwr_domain_on_finish		= mtk_plat_power_domain_on_finish,
	.pwr_domain_off			= mtk_plat_power_domain_off,
	.pwr_domain_suspend		= mtk_plat_power_domain_suspend,
	.pwr_domain_suspend_finish	= mtk_plat_power_domain_suspend_finish,
	.system_off			= mtk_plat_system_off,
	.system_reset			= mtk_plat_system_reset,
	.system_reset2			= mtk_plat_system_reset2,
	.validate_power_state		= plat_validate_power_state,
	.get_sys_suspend_power_state	= plat_get_sys_suspend_power_state,
};

int plat_setup_psci_ops(uintptr_t sec_entrypoint,
			const plat_psci_ops_t **psci_ops)
{
	*psci_ops = &plat_plat_pm_ops;
	secure_entrypoint = sec_entrypoint;
	return 0;
}

/*
 * The PSCI generic code uses this API to let the platform participate in state
 * coordination during a power management operation. It compares the platform
 * specific local power states requested by each cpu for a given power domain
 * and returns the coordinated target power state that the domain should
 * enter. A platform assigns a number to a local power state. This default
 * implementation assumes that the platform assigns these numbers in order of
 * increasing depth of the power state i.e. for two power states X & Y, if X < Y
 * then X represents a shallower power state than Y. As a result, the
 * coordinated target local power state for a power domain will be the minimum
 * of the requested local power states.
 */
plat_local_state_t plat_get_target_pwr_state(unsigned int lvl,
					     const plat_local_state_t *states,
					     unsigned int ncpu)
{
	plat_local_state_t target = PLAT_MAX_OFF_STATE, temp;

	assert(ncpu);

	do {
		temp = *states++;
		if (temp < target)
			target = temp;
	} while (--ncpu);

	return target;
}
