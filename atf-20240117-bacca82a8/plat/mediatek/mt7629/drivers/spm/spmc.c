/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <spmc.h>
#include <platform_def.h>

/* Infrasys configuration */
#define INFRA_TOPAXI_PROT_EN	0x220
#define INFRA_TOPAXI_PROT_STA1	0x228

int mtk_spm_cpu1_power_on(void)
{
	uint32_t val;

	/* enable register control */
	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_REGWR_CFG_KEY | SPM_REGWR_EN);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) & ~PWR_RST_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) | PWR_CLK_DIS_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) | PWR_ON_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) | PWR_ON_2ND_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	while (!(mmio_read_32(SPM_PWR_STATUS) & PWR_STATUS_CPU1) ||
	       !(mmio_read_32(SPM_PWR_STATUS_2ND) & PWR_STATUS_CPU1))
		;

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) & ~PWR_ISO_BIT &
	      ~PD_SLPB_CLAMP_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_L1_PDN) & ~MP0_CPU1_L1_PDN;
	mmio_write_32(SPM_MP0_CPU1_L1_PDN, val);

	while (mmio_read_32(SPM_MP0_CPU1_L1_PDN) & MP0_CPU1_L1_PDN_ACK)
		;

	udelay(1); /* Wait 1000ns for memory power ready */

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) & ~SRAM_CKISO_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) & ~PWR_CLK_DIS_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) | PWR_RST_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	return 0;
}

int mtk_spm_cpu1_power_off(void)
{
	uint32_t val;

	/* enable register control */
	mmio_write_32(SPM_POWERON_CONFIG_SET,
		      SPM_PROJECT_CODE | SPM_REGWR_EN);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) |
	      PWR_ISO_BIT | PD_SLPB_CLAMP_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) | SRAM_CKISO_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);;

	val = mmio_read_32(SPM_MP0_CPU1_L1_PDN) | MP0_CPU1_L1_PDN;
	mmio_write_32(SPM_MP0_CPU1_L1_PDN, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) & ~PWR_RST_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) | PWR_CLK_DIS_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	val = mmio_read_32(SPM_MP0_CPU1_PWR_CON) &
	      ~PWR_ON_BIT & ~PWR_ON_2ND_BIT;
	mmio_write_32(SPM_MP0_CPU1_PWR_CON, val);

	while ((mmio_read_32(SPM_PWR_STATUS) & PWR_STATUS_CPU1) ||
	       (mmio_read_32(SPM_PWR_STATUS_2ND) & PWR_STATUS_CPU1))
		;

	return 0;
}


struct scp_domain_data {
	uint32_t sta_mask;
	int ctl_offs;
	uint32_t sram_pdn_bits;
	uint32_t sram_pdn_ack_bits;
	uint32_t bus_prot_mask;
};

static struct scp_domain_data scp_domain_mt7629[] = {
	[MT7629_POWER_DOMAIN_ETHSYS] = {
		.sta_mask = PWR_STATUS_ETHSYS,
		.ctl_offs = SPM_ETHSYS_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bus_prot_mask = (BIT(3) | BIT(17)),
	},
	[MT7629_POWER_DOMAIN_HIF0] = {
		.sta_mask = PWR_STATUS_HIF0,
		.ctl_offs = SPM_HIF0_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bus_prot_mask = GENMASK(25, 24),
	},
	[MT7629_POWER_DOMAIN_HIF1] = {
		.sta_mask = PWR_STATUS_HIF1,
		.ctl_offs = SPM_HIF1_PWR_CON,
		.sram_pdn_bits = GENMASK(11, 8),
		.sram_pdn_ack_bits = GENMASK(15, 12),
		.bus_prot_mask = GENMASK(28, 26),
	},
};

static int mtk_spm_power_on(uint32_t on, struct scp_domain_data *data)
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

int mtk_spm_non_cpu_ctrl(uint32_t on, uint32_t num)
{
	uint32_t ret = -1;
	struct scp_domain_data *data;

	switch (num) {
	case MT7629_POWER_DOMAIN_ETHSYS:
		data = &scp_domain_mt7629[MT7629_POWER_DOMAIN_ETHSYS];
		ret = mtk_spm_power_on(on, data);
		break;
	case MT7629_POWER_DOMAIN_HIF0:
		data = &scp_domain_mt7629[MT7629_POWER_DOMAIN_HIF0];
		ret = mtk_spm_power_on(on, data);
		break;
	case MT7629_POWER_DOMAIN_HIF1:
		data = &scp_domain_mt7629[MT7629_POWER_DOMAIN_HIF1];
		ret = mtk_spm_power_on(on, data);
		break;
	default:
		INFO("No mapping MTCMOS(%d), ret = %d\n", num, ret);
		break;
	}

	return ret;
}
