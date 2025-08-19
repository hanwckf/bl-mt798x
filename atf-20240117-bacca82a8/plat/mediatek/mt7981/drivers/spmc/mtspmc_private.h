#ifndef _MTSPMC_PRIVATE_H_
#define _MTSPMC_PRIVATE_H_

uint64_t read_cpuectlr(void);
void write_cpuectlr(uint64_t);

/*
 * per_cpu/cluster helper
 */
struct per_cpu_reg {
	int cluster_addr;
	int cpu_stride;
};

#define per_cpu(cluster, cpu, reg) (reg[cluster].cluster_addr + (cpu << reg[cluster].cpu_stride))
#define per_cluster(cluster, reg) (reg[cluster].cluster_addr)

/* APB Module mcucfg */
#define MP0_CA7_CACHE_CONFIG		(MCUCFG_BASE + 0x000)
#define MP0_AXI_CONFIG			(MCUCFG_BASE + 0x02C)
#define MP0_MISC_CONFIG0		(MCUCFG_BASE + 0x030)
#define MP0_MISC_CONFIG1		(MCUCFG_BASE + 0x034)
#define MP0_MISC_CONFIG2		(MCUCFG_BASE + 0x038)
#define MP0_MISC_CONFIG_BOOT_ADDR(cpu)	(MCUCFG_BASE + 0x038 + ((cpu) * 8))
#define MP0_MISC_CONFIG3		(MCUCFG_BASE + 0x03C)
#define MP0_MISC_CONFIG9		(MCUCFG_BASE + 0x054)
#define MP0_CA7_MISC_CONFIG		(MCUCFG_BASE + 0x064)
#define CPUSYS0_CPU0_SPMC_CTL		(MCUCFG_BASE + 0x1c30)
#define CPUSYS0_CPU1_SPMC_CTL		(MCUCFG_BASE + 0x1c34)
#define CPUSYS0_CPU2_SPMC_CTL		(MCUCFG_BASE + 0x1c38)
#define CPUSYS0_CPU3_SPMC_CTL		(MCUCFG_BASE + 0x1c3C)

#define MCUCFG_MP0_MISC_CONFIG2         MP0_MISC_CONFIG2
#define MCUCFG_MP0_MISC_CONFIG3         MP0_MISC_CONFIG3

/* per_cpu registers for MCUCFG_MP?_MISC_CONFIG2 */
static const struct per_cpu_reg MCUCFG_BOOTADDR[] = {
	[0] = { .cluster_addr = MCUCFG_MP0_MISC_CONFIG2, .cpu_stride = 3 },
};

/* per_cpu registers for MCUCFG_MP?_MISC_CONFIG3 */
static const struct per_cpu_reg MCUCFG_INITARCH[] = {
	[0] = { .cluster_addr = MCUCFG_MP0_MISC_CONFIG3 },
};

#define cpu_sw_no_wait_for_q_channel	(1<<1)

#define MCUSYS_PROTECTEN_SET		(INFRACFG_AO_BASE + 0x2B4)
#define MCUSYS_PROTECTEN_CLR		(INFRACFG_AO_BASE + 0x2B8)
#define MCUSYS_PROTECTEN_STA1		(INFRACFG_AO_BASE + 0x2C0)

/* TOP misc register */
#define MCU_VPROC_CON			(TOP_MISC_BASE + 0x000)
#define MCUSYS_TOP_PWR_CON		(TOP_MISC_BASE + 0x004)
#define MP_TOP_PWR_CON			(TOP_MISC_BASE + 0x008)
#define MP0_TOP_PWR_CON			(TOP_MISC_BASE + 0x00C)
#define MP0_CPU0_PWR_CON		(TOP_MISC_BASE + 0x010)
#define MP0_CPU1_PWR_CON		(TOP_MISC_BASE + 0x014)
#define MP0_CPU2_PWR_CON		(TOP_MISC_BASE + 0x018)
#define MP0_CPU3_PWR_CON		(TOP_MISC_BASE + 0x01C)
#define MCU_SPMC_MODE_CON		(TOP_MISC_BASE + 0x020)
#define MP0_TOP_SPMC_PWR_CON		(TOP_MISC_BASE + 0x024)
#define MP0_CPU0_SPMC_PWR_CON		(TOP_MISC_BASE + 0x028)
#define MP0_CPU1_SPMC_PWR_CON		(TOP_MISC_BASE + 0x02C)
#define MP0_CPU2_SPMC_PWR_CON		(TOP_MISC_BASE + 0x030)
#define MP0_CPU3_SPMC_PWR_CON		(TOP_MISC_BASE + 0x034)
#define NETSYS_PWR_CON			(TOP_MISC_BASE + 0x080)
#define MCU_BUS_PROTECT			(TOP_MISC_BASE + 0x100)
#define MCU_BUS_PROTECT_STA		(TOP_MISC_BASE + 0x104)
#define MCU_MISC			(TOP_MISC_BASE + 0x108)
#define PCIE_PHY_I2C_ENABLE		(TOP_MISC_BASE + 0x110)
#define CONNSYS_MISC			(TOP_MISC_BASE + 0x114)
#define DEBUG_MON_SYS_SEL		(TOP_MISC_BASE + 0x120)
#define DEBUG_MON_SEL_BIT3_0		(TOP_MISC_BASE + 0x130)
#define DEBUG_MON_SEL_BIT7_4		(TOP_MISC_BASE + 0x134)
#define DEBUG_MON_SEL_BIT11_8		(TOP_MISC_BASE + 0x138)
#define DEBUG_MON_SEL_BIT15_12		(TOP_MISC_BASE + 0x13C)
#define DEBUG_MON_SEL_BIT19_16		(TOP_MISC_BASE + 0x140)
#define DEBUG_MON_SEL_BIT23_20		(TOP_MISC_BASE + 0x144)
#define DEBUG_MON_SEL_BIT27_24		(TOP_MISC_BASE + 0x148)
#define DEBUG_MON_SEL_BIT31_28		(TOP_MISC_BASE + 0x14C)
#define TOP_MISC_RSRV_ALL0_0		(TOP_MISC_BASE + 0x200)
#define TOP_MISC_RSRV_ALL0_1		(TOP_MISC_BASE + 0x204)
#define TOP_MISC_RSRV_ALL0_2		(TOP_MISC_BASE + 0x208)
#define TOP_MISC_RSRV_ALL0_3		(TOP_MISC_BASE + 0x20C)
#define TOP_MISC_RSRV_ALL1_0		(TOP_MISC_BASE + 0x210)
#define TOP_MISC_RSRV_ALL1_1		(TOP_MISC_BASE + 0x214)
#define TOP_MISC_RSRV_ALL1_2		(TOP_MISC_BASE + 0x218)
#define TOP_MISC_RSRV_ALL1_3		(TOP_MISC_BASE + 0x21C)

#define IDX_PROTECT_MP0_CACTIVE		0
#define IDX_PROTECT_ICC0_CACTIVE	1
#define IDX_PROTECT_ICD0_CACTIVE	2
#define IDX_PROTECT_L2C0_CACTIVE	3

#define IDX_PROTECT_MP1_CACTIVE		4
#define IDX_PROTECT_ICC1_CACTIVE	5
#define IDX_PROTECT_ICD1_CACTIVE	6
#define IDX_PROTECT_L2C1_CACTIVE	7

#define MP0_SNOOP_CTRL				(0x0C595000)
#define MP1_SNOOP_CTRL				(0x0C594000)
#define MPx_SNOOP_STATUS			(0x0C59000C)
#define MPx_SNOOP_ENABLE			(0x3)

#define MCUSYS_CPU_PWR_STA_MASK      (0x1 << 16)
#define MP_TOP_CPU_PWR_STA_MASK      (0x1 << 15)
#define MP0_CPUTOP_PWR_STA_MASK      (0x1 << 13)
#define MP0_CPU0_PWR_STA_MASK        (0x1 << 5)
#define MP0_CPU1_PWR_STA_MASK        (0x1 << 6)
#define MP0_CPU2_PWR_STA_MASK        (0x1 << 7)
#define MP0_CPU3_PWR_STA_MASK        (0x1 << 8)
#define MP1_CPUTOP_PWR_STA_MASK      (0x1 << 14)
#define MP1_CPU0_PWR_STA_MASK        (0x1 << 9)
#define MP1_CPU1_PWR_STA_MASK        (0x1 << 10)
#define MP1_CPU2_PWR_STA_MASK        (0x1 << 11)
#define MP1_CPU3_PWR_STA_MASK        (0x1 << 12)

#define MP0_CPU0_STANDBYWFI		(1U << 6)
#define MP0_CPU1_STANDBYWFI		(1U << 7)
#define MP0_CPU2_STANDBYWFI		(1U << 8)
#define MP0_CPU3_STANDBYWFI		(1U << 9)

#define MP0_SPMC_SRAM_DORMANT_EN            (1<<0)
#define MP1_SPMC_SRAM_DORMANT_EN            (1<<1)

#define PWR_ACK_2ND		(1U << 31)
#define PWR_ACK			(1U << 30)
#define SRAM_SLEEP_B_ACK	(1U << 28)
#define SRAM_PDN_ACK		(1U << 24)
#define SRAM_SLEEP_B		(1U << 12)
#define SRAM_PDN		(1U << 8)
#define SRAM_PD_SLPB_CLAMP	(1U << 7)
#define SRAM_ISOINT_B		(1U << 6)
#define SRAM_CKISO		(1U << 5)

#define PWR_CLK_DIS		(1U << 3)
#define RESETPWRON		(1U << 2)
#define PWR_ON			(1U << 1)
#define PWR_RST_B		(1U << 0)

#endif
