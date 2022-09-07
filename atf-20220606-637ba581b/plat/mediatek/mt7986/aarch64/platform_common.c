/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <common/bl_common.h>
#include <common/debug.h>
//#include <drivers/arm/cci.h>
#include <lib/mmio.h>
#include <lib/xlat_tables/xlat_tables.h>
#include <platform_def.h>
#include <mt7986_mmu.h>

#if 0
//static const int cci_map[] = {
//	CCI_CLUSTER0_SL_IFACE_IX,
//	CCI_CLUSTER1_SL_IFACE_IX
//};
#endif

/* Table of regions to map using the MMU.  */
const mmap_region_t plat_mmap[] = {
	MAP_SEC_SYSRAM,
	MAP_GIC,
	MAP_DEVICE0,
	MAP_DEVICE1,
	{ 0 }
};

/********************************a***********************************************
 * Macro generating the code for the function setting up the pagetables as per
 * the platform memory map & initialize the mmu, for the given exception level
 ******************************************************************************/
#define DEFINE_CONFIGURE_MMU_EL(_el)					\
	void plat_configure_mmu_el ## _el(unsigned long total_base,	\
					  unsigned long total_size,	\
					  unsigned long ro_start,	\
					  unsigned long ro_limit,	\
					  unsigned long coh_start,	\
					  unsigned long coh_limit)	\
	{								\
		mmap_add_region(total_base, total_base,			\
				total_size,				\
				MT_MEMORY | MT_RW | MT_SECURE);		\
		mmap_add_region(ro_start, ro_start,			\
				ro_limit - ro_start,			\
				MT_MEMORY | MT_RO | MT_SECURE);		\
		mmap_add_region(coh_start, coh_start,			\
				coh_limit - coh_start,			\
				MT_DEVICE | MT_RW | MT_SECURE);		\
		mmap_add(plat_mmap);					\
		init_xlat_tables();					\
									\
		enable_mmu_el ## _el(0);				\
	}


/* Define EL3 variants of the function initialising the MMU */
DEFINE_CONFIGURE_MMU_EL(3)

unsigned int plat_get_syscnt_freq2(void)
{
	return 13000000;
}

#if 0
void plat_cci_init(void)
{
	/* Initialize CCI driver */
	cci_init(CCI_BASE, cci_map, ARRAY_SIZE(cci_map));
	/* Eable non-secure access*/
	mmio_write_32(CCI_BASE + CCI_SEC_ACCESS , 0x1);
}

void plat_cci_enable(void)
{
	/*
	 * Enable CCI coherency for this cluster.
	 * No need for locks as no other cpu is active at the moment.
	 */
	cci_enable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr()));
}

void plat_cci_disable(void)
{
	cci_disable_snoop_dvm_reqs(MPIDR_AFFLVL1_VAL(read_mpidr()));
}
#endif

