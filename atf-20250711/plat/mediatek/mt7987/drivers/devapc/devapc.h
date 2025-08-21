/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 */

#ifndef _DEVAPC_H_
#define _DEVAPC_H_

#include <stdint.h>
#include <platform_def.h>

/******************************************************************************
 * FUNCTION DEFINITION
 ******************************************************************************/
void infra_devapc_init(void);

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

enum DEVAPC_TRANSACTION {
	DAPC_NSEC = 0,
	DAPC_SEC
};

enum DEVAPC_TYPE {
	DEVAPC_INFRA_AO,
	DEVAPC_FMEM_AO
};

enum DEVAPC_INFRA_AO_SYS_ID {
	SYS_ID_INFRA_AO_SYS0 = 0,
	SYS_ID_INFRA_AO_SYS1,
	SYS_ID_INFRA_AO_MAX
};

/* Slave Num */
enum DEVAPC_SLAVE_NUM {
	SLAVE_NUM_INFRA_AO_SYS0 = 127,
	SLAVE_NUM_INFRA_AO_SYS1 = 1
};

enum DEVAPC_MAS_NUM {
	MAS_NUM_INFRA_AO = 12,
	MAS_NUM_FMEM_AO = 5
};

enum DEVAPC_DOM_BIT {
	DOM_BIT_INFRA_AO = 4,
	DOM_BIT_FMEM_AO = 4
};

enum DEVAPC_REMAP_DOM_BIT {
	REMAP_DOM_BIT_INFRA_AO = 2,
	REMAP_DOM_BIT_FMEM_AO = 8
};

enum DEVAPC_SYS_DOM_NUM {
	DOM_NUM_INFRA_AO_SYS0 = 16,
	DOM_NUM_INFRA_AO_SYS1 = 4
};

struct APC_PERI_DOM_16 {
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

struct APC_PERI_DOM_4 {
	unsigned char d0_permission;
	unsigned char d1_permission;
	unsigned char d2_permission;
	unsigned char d3_permission;
};

struct devapc_remap_table {
	unsigned char dom;
	unsigned char remap_dom;
};

struct devapc_ao_master {
	int dom_idx;
	int sec_idx;
	unsigned char dom;
	unsigned char sec;
};

struct devapc_ao_system {
	uint32_t id;
	uint32_t mod_num;
	const void *modules;
	uint32_t dom_num;
};

struct devapc_desc {
	uintptr_t ao_base;
	uintptr_t pd_base;

	uint32_t dom_bit;
	uint32_t remap_dom_bit;
	const struct devapc_remap_table *remap_table;

	uint32_t mas_num;
	const struct devapc_ao_master *mas;

	uint32_t sys_num;
	const struct devapc_ao_system *sys;
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

#define DAPC_MAS_ATTR(MAS_NAME, DOM_IDX, SEC_IDX, DOMAIN, SECURE) \
	{(int)(DOM_IDX), (int)(SEC_IDX), \
	(unsigned char)(DOMAIN), (unsigned char)(SECURE)}

#define DAPC_DOM_REMAP(DOM, REMAP_DOM) \
	{(unsigned char)(DOM), (unsigned char)(REMAP_DOM)}

/******************************************************************************
 * UTILITY DEFINITION
 ******************************************************************************/
#define devapc_writel(VAL, REG)		mmio_write_32((uintptr_t)REG, VAL)
#define devapc_readl(REG)		mmio_read_32((uintptr_t)REG)

#define APC_CON_SEN			U(1)

#define AO_APC_CON(base)		((base) + DEVAPC_AO_APC_CON)
#define AO_APC_CON1(base)		((base) + DEVAPC_AO_APC_CON1)
#define AO_MAS_DOM(base)		((base) + DEVAPC_AO_MAS_DOM_0)
#define AO_MAS_SEC(base)		((base) + DEVAPC_AO_MAS_SEC_0)
#define AO_DOM_REMAP(base)		((base) + DEVAPC_AO_DOM_REMAP_0_0)
#define AO_SYS0_D0_APC(base)		((base) + DEVAPC_AO_SYS0_D0_APC_0)
#define AO_SYS0_APC_LOCK(base)		((base) + DEVAPC_AO_SYS0_APC_LOCK_0)

/******************************************************************************/
/* Device APC for AO */
#define DEVAPC_AO_APC_CON			(0x0F00)
#define DEVAPC_AO_APC_CON1			(0x0F04)
#define DEVAPC_AO_MAS_DOM_0			(0x0900)
#define DEVAPC_AO_MAS_SEC_0			(0x0A00)
#define DEVAPC_AO_DOM_REMAP_0_0			(0x0800)
#define DEVAPC_AO_SYS0_D0_APC_0			(0x0000)
#define DEVAPC_AO_SYS0_APC_LOCK_0		(0x0C00)

/******************************************************************************/
/* Device APC for PDN */
#define DEVAPC_PDN_APC_CON			(0x0F00)

/******************************************************************************/

/******************************************************************************
 * Variable DEFINITION
 ******************************************************************************/
#define MOD_NO_IN_1_DEVAPC			16
#define MAS_NO_IN_1_MAS_DOM			4

#endif /* _DEVAPC_H_ */
