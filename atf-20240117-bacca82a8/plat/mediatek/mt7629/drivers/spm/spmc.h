/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SPM_H
#define SPM_H

/*
 * for SPM register control
 */
#define SPM_POWERON_CONFIG_SET		(SPM_BASE + 0x0000)
#define SPM_MP0_CPU1_PWR_CON		(SPM_BASE + 0x0218)
#define SPM_MP0_CPU1_L1_PDN		(SPM_BASE + 0x0264)
#define SPM_PWR_STATUS			(SPM_BASE + 0x060c)
#define SPM_PWR_STATUS_2ND		(SPM_BASE + 0x0610)
#define SPM_SLEEP_TIMER_STA		(SPM_BASE + 0x0720)
#define SPM_ETHSYS_PWR_CON		0x2e0
#define SPM_HIF0_PWR_CON		0x2e4
#define SPM_HIF1_PWR_CON		0x2e8
#define SPM_PROJECT_CODE		0xb16
#define SPM_REGWR_EN			BIT(0)
#define SPM_REGWR_CFG_KEY		(SPM_PROJECT_CODE << 16)

#define PWR_RST_BIT			BIT(0)
#define PWR_ISO_BIT			BIT(1)
#define PWR_ON_BIT			BIT(2)
#define PWR_ON_2ND_BIT			BIT(3)
#define PWR_CLK_DIS_BIT			BIT(4)
#define SRAM_CKISO_BIT			BIT(5)
#define SRAM_ISOINT_BIT			BIT(6)
#define PD_SLPB_CLAMP_BIT		BIT(7)

#define MP0_CPU1_L1_PDN_ACK		BIT(8)
#define MP0_CPU1_L1_PDN			BIT(0)
#define PWR_STATUS_CPU1			BIT(12)
#define PWR_STATUS_ETHSYS		BIT(24)
#define PWR_STATUS_HIF0			BIT(25)
#define PWR_STATUS_HIF1			BIT(26)
#define MP0_CPU1_STANDBYWFI		BIT(17)

enum {
	MT7629_POWER_DOMAIN_ETHSYS,
	MT7629_POWER_DOMAIN_HIF0,
	MT7629_POWER_DOMAIN_HIF1
};

int mtk_spm_non_cpu_ctrl(uint32_t on, uint32_t num);
int mtk_spm_cpu1_power_on(void);
int mtk_spm_cpu1_power_off(void);

#endif /* SPM_H */
