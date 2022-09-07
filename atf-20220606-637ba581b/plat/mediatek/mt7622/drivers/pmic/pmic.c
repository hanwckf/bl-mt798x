/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <platform_def.h>
#include "pmic.h"
#include "pmic_wrap_init.h"

static void pmic_read_interface(uint32_t reg, uint32_t *val,
				uint32_t mask, uint32_t shift)
{
	uint32_t pmic_reg;

	pwrap_read(reg, &pmic_reg);

	pmic_reg &= (mask << shift);
	*val = (pmic_reg >> shift);
}

static void pmic_config_interface(uint32_t reg, uint32_t val,
				  uint32_t mask, uint32_t shift)
{
	uint32_t pmic_reg;

	pwrap_read(reg, &pmic_reg);

	pmic_reg &= ~(mask << shift);
	pmic_reg |= (val << shift);

	pwrap_write(reg, pmic_reg);
}

void mtk_pmic_init(void)
{
	uint32_t val = 0;

	/* read chip id */
	pmic_read_interface(MT6380_IO_CTRL, &val, 1, 28);
	INFO("PMIC: MediaTek MT6380 E%u\n", val ? 2 : 3);

	pmic_config_interface(MT6380_ANA_CTRL_1, 0x72, 0xfe, 1);
	/* set vsram 0x1c[15:13], 1.4V = 0, 1.35V = 1, 1.30V = 2,
	 * 1.25V = 3, 1.20V = 4, 1.15V = 5, 1.10V = 6, 1.05V = 7
	 */
	pmic_config_interface(MT6380_MLDO_CON_0, 1, 0x7, 13);

	/* set vrf fspwm mode */
	pmic_config_interface(MT6380_SIDO_CON_0, 1, 1, 15);

	/* set vrf trim voltage */
	pmic_config_interface(0x104, 0x11a, 0xffffffff, 0);
	pmic_read_interface(0x11a, &val, 0xf, 9);
	pmic_config_interface(MT6380_SIDO_CON_0, val, 0xf, 3);
	pmic_read_interface(0x11a, &val, 1, 8);
	pmic_config_interface(MT6380_EFU_CTRL_3, val, 1, 1);
}
