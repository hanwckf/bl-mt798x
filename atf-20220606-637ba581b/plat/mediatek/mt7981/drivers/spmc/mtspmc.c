/*
 * Copyright (c) 2015,  ARM Limited and Contributors. All rights reserved.
 *
 * Redistribution and use in source and binary forms,  with or without
 * modification,  are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice,  this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * Neither the name of ARM nor the names of its contributors may be used
 * to endorse or promote products derived from this software without specific
 * prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES,  INCLUDING,  BUT NOT LIMITED TO,  THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL DAMAGES (INCLUDING,  BUT NOT LIMITED TO,  PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  DATA,  OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,  WHETHER IN
 * CONTRACT,  STRICT LIABILITY,  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,  EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <assert.h>
#include <lib/mmio.h>
#include <mtspmc.h>
#include <mtspmc_private.h>
#include <plat_private.h>
#include <platform_def.h>

unsigned int cpu_bitmask;

#if CONFIG_SPMC_SPARK == 1
static void spark_set_ldo(int cluster, int cpu)
{
	uintptr_t reg;
	unsigned int sparkvretcntrl = 0x3f;	/* 34=.5 3f=.6 */

	assert(cpu >= 0 && cpu < 4);

	VERBOSE("SPMC: sparkvretcntrl for cluster %u CPU %u is = 0x%x\n",
		cluster, cpu, sparkvretcntrl);
	reg = per_cluster(cluster, MCUCFG_SPARK2LDO);

	/*
	 * each core in LITTLE cluster can control its
	 * spark voltage
	 */
	mmio_clrsetbits_32(reg, 0x3f << (cpu << 3),
			   sparkvretcntrl << (cpu << 3));
}

static void spark_set_retention(int tick)
{
	uint64_t cpuectlr;

	cpuectlr = read_cpuectlr();
	cpuectlr &= ~0x7ULL;
	cpuectlr |= tick & 0x7;
	write_cpuectlr(cpuectlr);
}

void spark_enable(int cluster, int cpu)
{
	uintptr_t reg;

	/* only L cluster (cluster0) in MT6765 has SPARK */
	if (cluster)
		return;

	spark_set_ldo(cluster, cpu);
	spark_set_retention(1);

	reg = per_cpu(cluster, cpu, MCUCFG_SPARK);
	mmio_setbits_32(reg, SW_SPARK_EN);

	VERBOSE("SPMC: spark enable for cluster %u CPU %u, SW_SPARK_EN(0x%x) = 0x%x\n",
		cluster, cpu, reg, mmio_read_32(reg));
}

void spark_disable(int cluster, int cpu)
{
	uintptr_t reg;

	/* only L cluster (cluster0) in MT6765 has SPARK */
	if (cluster)
		return;

	spark_set_retention(0);

	reg = per_cpu(cluster, cpu, MCUCFG_SPARK);
	mmio_clrbits_32(reg, SW_SPARK_EN);

	VERBOSE("SPMC: spark disable for cluster %u CPU %u, SW_SPARK_EN(0x%x) = 0x%x\n",
		cluster, cpu, reg, mmio_read_32(reg));
}
#else /* CONFIG_SPMC_SPARK == 1 */
void spark_enable(int cluster, int cpu)
{
}

void spark_disable(int cluster, int cpu)
{
}
#endif /* CONFIG_SPMC_SPARK == 0 */

void set_cpu_retention_control(int retention_value)
{
	uint64_t cpuectlr;

	cpuectlr = read_cpuectlr();
	cpuectlr = ((cpuectlr >> 3) << 3);
	cpuectlr |= retention_value;
	write_cpuectlr(cpuectlr);
}

void mcucfg_set_bootaddr(int cluster, int cpu, uintptr_t bootaddr)
{
	mmio_write_32(per_cpu(cluster, cpu, MCUCFG_BOOTADDR), bootaddr);
}

uintptr_t mcucfg_get_bootaddr(int cluster, int cpu)
{
	return mmio_read_32(per_cpu(cluster, cpu, MCUCFG_BOOTADDR));
}

void mcucfg_init_archstate(int cluster, int cpu, int arm64)
{
	uintptr_t reg;

	reg = per_cluster(cluster, MCUCFG_INITARCH);

	if (arm64)
		mmio_setbits_32(reg, 1 << (12 + cpu));
	else
		mmio_clrbits_32(reg, 1 << (12 + cpu));
}

int spmc_init(void)
{
	int err = 0;

#if CONFIG_SPMC_MODE != 0
	/* De-assert Bypass SPMC  0: SPMC mode	1: Legacy mode */
	mmio_write_32(MCU_SPMC_MODE_CON, 0x0);
	INFO("SPMC: Changed to SPMC mode\n");
#endif

	/* MP0 SPMC power Ctrl signals */
	mmio_clrbits_32(MP0_CPU0_SPMC_PWR_CON, PWR_RST_B);
	mmio_clrbits_32(MP0_CPU1_SPMC_PWR_CON, PWR_RST_B);
	mmio_clrbits_32(MP0_CPU2_SPMC_PWR_CON, PWR_RST_B);
	mmio_clrbits_32(MP0_CPU3_SPMC_PWR_CON, PWR_RST_B);

	mmio_clrbits_32(MP0_TOP_SPMC_PWR_CON, PWR_RST_B);
	mmio_clrbits_32(MP0_TOP_SPMC_PWR_CON, PWR_CLK_DIS);

	cpu_bitmask = 1;

	return err;
}

int spmc_cpu_corex_onoff(int linear_id, int state, int mode)
{
	int err = 0;
	unsigned int CPUSYSx_CPUx_SPMC_CTL = 0, MPx_CPUx_PWR_CON = 0,
		     MPx_CPUx_STANDBYWFI = 0, MPx_CPUx_PWR_STA_MASK = 0;

	switch (linear_id) {
	case 0:
		CPUSYSx_CPUx_SPMC_CTL = CPUSYS0_CPU0_SPMC_CTL;
		MPx_CPUx_STANDBYWFI = MP0_CPU0_STANDBYWFI;
		MPx_CPUx_PWR_CON = MP0_CPU0_SPMC_PWR_CON;
		break;
	case 1:
		CPUSYSx_CPUx_SPMC_CTL = CPUSYS0_CPU1_SPMC_CTL;
		MPx_CPUx_STANDBYWFI = MP0_CPU1_STANDBYWFI;
		MPx_CPUx_PWR_CON = MP0_CPU1_SPMC_PWR_CON;
		break;
	case 2:
		CPUSYSx_CPUx_SPMC_CTL = CPUSYS0_CPU2_SPMC_CTL;
		MPx_CPUx_STANDBYWFI = MP0_CPU2_STANDBYWFI;
		MPx_CPUx_PWR_CON = MP0_CPU2_SPMC_PWR_CON;
		break;
	case 3:
		CPUSYSx_CPUx_SPMC_CTL = CPUSYS0_CPU3_SPMC_CTL;
		MPx_CPUx_STANDBYWFI = MP0_CPU3_STANDBYWFI;
		MPx_CPUx_PWR_CON = MP0_CPU3_SPMC_PWR_CON;
		break;
	default:
		INFO("SPMC: CPU%d not exists for power %s\n", linear_id,
		     state == STA_POWER_DOWN ? "off" : "on");
	}

	MPx_CPUx_PWR_STA_MASK = PWR_ACK;

	if (state == STA_POWER_DOWN) {
		if (!(cpu_bitmask & (1 << linear_id))) {
			INFO("SPMC: CPU%d is already off, CPU bitmask = 0x%x\n",
			     linear_id, cpu_bitmask);
			return 0;
		}
		if (mode == MODE_AUTO_SHUT_OFF) {
			mmio_clrbits_32(CPUSYSx_CPUx_SPMC_CTL,
					cpu_sw_no_wait_for_q_channel);
			set_cpu_retention_control(1);
		} else {
			mmio_setbits_32(CPUSYSx_CPUx_SPMC_CTL,
					cpu_sw_no_wait_for_q_channel);

			while (!(mmio_read_32(MCU_MISC) & MPx_CPUx_STANDBYWFI))
				;
		}

		mmio_clrbits_32(MPx_CPUx_PWR_CON, PWR_ON);

		if (mode == MODE_SPMC_HW) {
			while (mmio_read_32(MPx_CPUx_PWR_CON) &
			       MPx_CPUx_PWR_STA_MASK)
				;
			VERBOSE("SPMC: MPx_CPUx_PWR_CON_0x%x = 0x%x\n",
				MPx_CPUx_PWR_CON,
				mmio_read_32(MPx_CPUx_PWR_CON));
		}

		cpu_bitmask &= ~(1 << linear_id);
	} else {
		mmio_setbits_32(MPx_CPUx_PWR_CON, PWR_ON);
		VERBOSE("SPMC: MPx_CPUx_PWR_CON_0x%x = 0x%x\n",
			MPx_CPUx_PWR_CON, mmio_read_32(MPx_CPUx_PWR_CON));

		while ((mmio_read_32(MPx_CPUx_PWR_CON) & MPx_CPUx_PWR_STA_MASK) !=
		       MPx_CPUx_PWR_STA_MASK)
			;

		cpu_bitmask |= (1 << linear_id);

		VERBOSE("SPMC: CPU bitmask = 0x%x\n", cpu_bitmask);
	}

	return err;
}
