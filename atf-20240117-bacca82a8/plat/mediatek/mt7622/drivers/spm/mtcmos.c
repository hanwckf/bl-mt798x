/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <mtcmos.h>
#include <platform_def.h>
#include <spm.h>

#define SPM_ETHSYS_PWR_CON	0x2e0
#define SPM_HIF0_PWR_CON	0x2e4
#define SPM_HIF1_PWR_CON	0x2e8

#define PWR_STATUS_ETHSYS	BIT(24)
#define PWR_STATUS_HIF0		BIT(25)
#define PWR_STATUS_HIF1		BIT(26)

/* Infrasys configuration */
#define INFRA_TOPAXI_PROT_EN    0x220
#define INFRA_TOPAXI_PROT_STA1  0x228

struct scp_domain_data {
	uint32_t sta_mask;
	int ctl_offs;
	uint32_t sram_pdn_bits;
	uint32_t sram_pdn_ack_bits;
	uint32_t bus_prot_mask;
};

static struct scp_domain_data scp_domain_mt7622[] = {
	[MT7622_POWER_DOMAIN_ETHSYS] = {
		.sta_mask = PWR_STATUS_ETHSYS,
		.ctl_offs = SPM_ETHSYS_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bus_prot_mask = (BIT(3) | BIT(17)),
	},
	[MT7622_POWER_DOMAIN_HIF0] = {
		.sta_mask = PWR_STATUS_HIF0,
		.ctl_offs = SPM_HIF0_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bus_prot_mask = GENMASK(25, 24),
	},
	[MT7622_POWER_DOMAIN_HIF1] = {
		.sta_mask = PWR_STATUS_HIF1,
		.ctl_offs = SPM_HIF1_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bus_prot_mask = GENMASK(28, 26),
	},
};

void mtcmos_little_cpu_on(void)
{
	/* enable register control */
	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_REGWR_CFG_KEY | SPM_REGWR_EN);

	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, PWR_ON_BIT);
	udelay(1);
	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, PWR_ON_2ND_BIT);

	while ((mmio_read_32(SPM_PWR_STATUS) & MP0_CPU1) != MP0_CPU1 ||
	       (mmio_read_32(SPM_PWR_STATUS_2ND) & MP0_CPU1) != MP0_CPU1)
		continue;

	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, PWR_ISO_BIT);
	mmio_clrbits_32(SPM_CA7_CPU1_L1_PDN, L1_PDN);

	while (mmio_read_32(SPM_CA7_CPU1_L1_PDN) & L1_PDN_ACK)
		continue;

	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, SRAM_ISOINT_BIT);
	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, SRAM_CKISO_BIT);
	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, PWR_CLK_DIS_BIT);
	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, PWR_RST_BIT);
}

void mtcmos_little_cpu_off(void)
{
	/* enable register control */
	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_REGWR_CFG_KEY | SPM_REGWR_EN);

	while (!(mmio_read_32(SPM_SLEEP_TIMER_STA) & MP0_CPU1_STANDBYWFI))
		continue;

	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, PWR_ISO_BIT);
	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, SRAM_CKISO_BIT);
	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, SRAM_ISOINT_BIT);
	mmio_setbits_32(SPM_CA7_CPU1_L1_PDN, L1_PDN);

	while (!(mmio_read_32(SPM_CA7_CPU1_L1_PDN) & L1_PDN_ACK))
		continue;

	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, PWR_RST_BIT);
	mmio_setbits_32(SPM_CA7_CPU1_PWR_CON, PWR_CLK_DIS_BIT);
	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, PWR_ON_BIT);
	mmio_clrbits_32(SPM_CA7_CPU1_PWR_CON, PWR_ON_2ND_BIT);

	while ((mmio_read_32(SPM_PWR_STATUS) & MP0_CPU1) ||
	       (mmio_read_32(SPM_PWR_STATUS_2ND) & MP0_CPU1))
		continue;
}

static int mtcmos_power_on(uint32_t on, struct scp_domain_data *data)
{
	uintptr_t ctl_addr = SPM_BASE + data->ctl_offs;
	uint32_t val, pdn_ack = data->sram_pdn_ack_bits;

	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_REGWR_CFG_KEY | SPM_REGWR_EN);

	val = mmio_read_32(ctl_addr);
	val |= PWR_ON_BIT;
	mmio_write_32(ctl_addr, val);

	val |= PWR_ON_2ND_BIT;
	mmio_write_32(ctl_addr, val);

	while (!(mmio_read_32(SPM_PWR_STATUS) & data->sta_mask) ||
	       !(mmio_read_32(SPM_PWR_STATUS_2ND) & data->sta_mask))
		continue;

	val &= ~PWR_CLK_DIS_BIT;
	mmio_write_32(ctl_addr, val);

	val &= ~PWR_ISO_BIT;
	mmio_write_32(ctl_addr, val);

	val |= PWR_RST_BIT;
	mmio_write_32(ctl_addr, val);

	val &= ~data->sram_pdn_bits;
	mmio_write_32(ctl_addr, val);

	while ((mmio_read_32(ctl_addr) & pdn_ack))
		continue;

	if (data->bus_prot_mask) {
		mmio_clrbits_32(INFRACFG_AO_BASE + INFRA_TOPAXI_PROT_EN,
				data->bus_prot_mask);
		while ((mmio_read_32(INFRACFG_AO_BASE + INFRA_TOPAXI_PROT_STA1) &
			data->bus_prot_mask))
			continue;
	}

	return 0;
}

int mtcmos_non_cpu_ctrl(uint32_t on, uint32_t num)
{
	uint32_t ret = -1;
	struct scp_domain_data *data;

	switch (num) {
	case MT7622_POWER_DOMAIN_ETHSYS:
		data = &scp_domain_mt7622[MT7622_POWER_DOMAIN_ETHSYS];
		ret = mtcmos_power_on(on, data);
		break;
	case MT7622_POWER_DOMAIN_HIF0:
		data = &scp_domain_mt7622[MT7622_POWER_DOMAIN_HIF0];
		ret = mtcmos_power_on(on, data);
		break;
	case MT7622_POWER_DOMAIN_HIF1:
		data = &scp_domain_mt7622[MT7622_POWER_DOMAIN_HIF1];
		ret = mtcmos_power_on(on, data);
		break;
	default:
		INFO("No mapping MTCMOS(%d), ret = %d\n", num, ret);
		break;
	}

	return ret;
}
