/*
 * Copyright (c) 2020, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _DEVAPC_H_
#define _DEVAPC_H_

#include <stdint.h>
#include <platform_def.h>

/******************************************************************************
 * FUNCTION DEFINITION
 ******************************************************************************/
void devapc_init(void);

/******************************************************************************
 * STRUCTURE DEFINITION
 ******************************************************************************/
enum DEVAPC_PERM_TYPE {
	NO_PROTECTION = 0,
	SEC_RW_ONLY,
	SEC_RW_NS_R,
	FORBIDDEN,
	PERM_NUM,
};

enum DOMAIN_ID {
	DOMAIN_0 = 0,
	DOMAIN_1,
	DOMAIN_2,
	DOMAIN_3,
	DOMAIN_4,
	DOMAIN_5,
	DOMAIN_6,
	DOMAIN_7,
	DOMAIN_8,
	DOMAIN_9,
	DOMAIN_10,
	DOMAIN_11,
	DOMAIN_12,
	DOMAIN_13,
	DOMAIN_14,
	DOMAIN_15,
};

/* Slave Type */
enum DEVAPC_SYS_INDEX {
	DEVAPC_SYS0 = 0,
	DEVAPC_SYS1,
};

enum DEVAPC_SLAVE_TYPE {
	SLAVE_TYPE_INFRA_AO_SYS0 = 0,
	SLAVE_TYPE_INFRA_AO_SYS1,
};

/* Slave Num */
enum DEVAPC_SLAVE_NUM {
	SLAVE_NUM_INFRA_AO_SYS0 = 115,
	SLAVE_NUM_INFRA_AO_SYS1 = 1,
};

enum DEVAPC_SYS_DOM_NUM {
	DOM_NUM_INFRA_AO_SYS0 = 16,
	DOM_NUM_INFRA_AO_SYS1 = 4,
};

struct APC_INFRA_PERI_DOM_16 {
	unsigned char d0_permission;
	unsigned char d1_permission;
	unsigned char d2_permission;
	unsigned char d3_permission;
	unsigned char d4_permission;
	unsigned char d5_permission;
	unsigned char d6_permission;
	unsigned char d7_permission;
	unsigned char d8_permission;
	unsigned char d9_permission;
	unsigned char d10_permission;
	unsigned char d11_permission;
	unsigned char d12_permission;
	unsigned char d13_permission;
	unsigned char d14_permission;
	unsigned char d15_permission;
};

struct APC_INFRA_PERI_DOM_8 {
	unsigned char d0_permission;
	unsigned char d1_permission;
	unsigned char d2_permission;
	unsigned char d3_permission;
	unsigned char d4_permission;
	unsigned char d5_permission;
	unsigned char d6_permission;
	unsigned char d7_permission;
};

struct APC_INFRA_PERI_DOM_4 {
	unsigned char d0_permission;
	unsigned char d1_permission;
	unsigned char d2_permission;
	unsigned char d3_permission;
};

#define DAPC_INFRA_AO_SYS0_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3, PERM_ATTR4, PERM_ATTR5, \
		PERM_ATTR6, PERM_ATTR7, PERM_ATTR8, PERM_ATTR9, \
		PERM_ATTR10, PERM_ATTR11, PERM_ATTR12, PERM_ATTR13, \
		PERM_ATTR14, PERM_ATTR15) \
	{(unsigned char)PERM_ATTR0, (unsigned char)PERM_ATTR1, \
	(unsigned char)PERM_ATTR2, (unsigned char)PERM_ATTR3, \
	(unsigned char)PERM_ATTR4, (unsigned char)PERM_ATTR5, \
	(unsigned char)PERM_ATTR6, (unsigned char)PERM_ATTR7, \
	(unsigned char)PERM_ATTR8, (unsigned char)PERM_ATTR9, \
	(unsigned char)PERM_ATTR10, (unsigned char)PERM_ATTR11, \
	(unsigned char)PERM_ATTR12, (unsigned char)PERM_ATTR13, \
	(unsigned char)PERM_ATTR14, (unsigned char)PERM_ATTR15}

#define DAPC_INFRA_AO_SYS1_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3) \
	{(unsigned char)PERM_ATTR0, (unsigned char)PERM_ATTR1, \
	(unsigned char)PERM_ATTR2, (unsigned char)PERM_ATTR3}

/******************************************************************************
 * UTILITY DEFINITION
 ******************************************************************************/
#define devapc_writel(VAL, REG)		mmio_write_32((uintptr_t)REG, VAL)
#define devapc_readl(REG)		mmio_read_32((uintptr_t)REG)

/******************************************************************************/
/* Device APC for FMEM_AO */
#define DEVAPC_FMEM_AO_APC_CON			(DEVAPC_FMEM_AO_BASE + 0x0F00)
#define DEVAPC_FMEM_AO_MAS_SEC_0		(DEVAPC_FMEM_AO_BASE + 0x0A00)

/******************************************************************************/
/* Device APC for INFRA_AO */
#define DEVAPC_INFRA_AO_APC_CON			(DEVAPC_INFRA_AO_BASE + 0x0F00)
#define DEVAPC_INFRA_AO_MAS_SEC_0		(DEVAPC_INFRA_AO_BASE + 0x0A00)

#define DEVAPC_INFRA_AO_SYS0_D0_APC_0		(DEVAPC_INFRA_AO_BASE + 0x0000)
#define DEVAPC_INFRA_AO_SYS1_D0_APC_0		(DEVAPC_INFRA_AO_BASE + 0x1000)

/******************************************************************************/
/* Device APC for INFRA_PDN */
#define DEVAPC_INFRA_PDN_APC_CON		(DEVAPC_INFRA_PDN_BASE + 0x0F00)

/******************************************************************************/

/******************************************************************************
 * Variable DEFINITION
 ******************************************************************************/
#define MOD_NO_IN_1_DEVAPC              16

#endif /* _DEVAPC_H_ */
