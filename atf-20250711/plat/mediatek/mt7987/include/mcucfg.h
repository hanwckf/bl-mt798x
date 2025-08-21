/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 */

#ifndef MT7987_MCUCFG_H
#define MT7987_MCUCFG_H

#include <platform_def.h>
#include <stdint.h>
#include <mt7987_def.h>

#define mcucfg_reg(x)	(MCUCFG_BASE + mcucfg_reg_offset[x])

enum mcucfg_reg_name {
	MCUCFG_INITARCH,
	MCUCFG_BOOTADDR0_H,
	MCUCFG_BOOTADDR0_L,
	MCUCFG_BOOTADDR1_H,
	MCUCFG_BOOTADDR1_L,
	MCUCFG_BOOTADDR2_H,
	MCUCFG_BOOTADDR2_L,
	MCUCFG_BOOTADDR3_H,
	MCUCFG_BOOTADDR3_L,
	MCUCFG_L2CTAG,
	MCUCFG_L2CCFG,
};

static const enum mcucfg_reg_name MCUCFG_BOOTADDR_H[] = {
	MCUCFG_BOOTADDR0_H,
	MCUCFG_BOOTADDR1_H,
	MCUCFG_BOOTADDR2_H,
	MCUCFG_BOOTADDR3_H,
};

static const enum mcucfg_reg_name MCUCFG_BOOTADDR_L[] = {
	MCUCFG_BOOTADDR0_L,
	MCUCFG_BOOTADDR1_L,
	MCUCFG_BOOTADDR2_L,
	MCUCFG_BOOTADDR3_L,
};

static const unsigned int mcucfg_reg_offset[] = {
	[MCUCFG_INITARCH] = 0x3c,
	[MCUCFG_BOOTADDR0_H] = 0x3c,
	[MCUCFG_BOOTADDR0_L] = 0x38,
	[MCUCFG_BOOTADDR1_H] = 0x44,
	[MCUCFG_BOOTADDR1_L] = 0x40,
	[MCUCFG_BOOTADDR2_H] = 0x4c,
	[MCUCFG_BOOTADDR2_L] = 0x48,
	[MCUCFG_BOOTADDR3_H] = 0x54,
	[MCUCFG_BOOTADDR3_L] = 0x50,
	[MCUCFG_L2CTAG] = 0x3c,
	[MCUCFG_L2CCFG] = 0x7f0,
};

/* cpu boot mode */
#define MCUCFG_INITARCH_SHIFT 12
#define MCUCFG_BOOTADDR_H_MASK 0xf
#define MCUCFG_BOOTADDR_L_MASK 0xfffffffc

#endif /* MT7987_MCUCFG_H */
