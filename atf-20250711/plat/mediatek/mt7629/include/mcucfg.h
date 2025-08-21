/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MCUCFG_H
#define MCUCFG_H

#include <stdint.h>
#include <mt7629_def.h>

#define MCU_AXI_DIV			(MCUCFG_BASE + 0x640)
#define AXI_DIV_MSK			GENMASK(4, 0)
#define AXI_DIV_SEL(x)			(x)

#define MCU_BUS_MUX			(MCUCFG_BASE + 0x7c0)
#define MCU_BUS_MSK			GENMASK(10, 9)
#define MCU_BUS_SEL(x)			((x) << 9)

/* INFRACFG_AO_BASE */
#define SRAMROM_BOOT_ADDR	(INFRACFG_AO_BASE + 0x800)
#define SW_ROM_PD		    BIT(31)
#define SRAMROM_BASE		(0x10202000)
#define BOOT_META0		    (SRAMROM_BASE + 0x034)




#endif /* MCUCFG_H */
