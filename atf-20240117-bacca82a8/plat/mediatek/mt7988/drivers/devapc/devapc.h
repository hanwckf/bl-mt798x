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
void infra_devapc_init(void);

void tops_devapc_init(void);

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

enum DEVAPC_TYPE {
	DAPC_TYPE_FMEM_AO = 0,
	DAPC_TYPE_INFRA_AO,
	DAPC_TYPE_TOPS_PERI_BUS_AO_PDN,
	DAPC_TYPE_TOPS_BUS_AO_PDN,
	DAPC_TYPE_CLUST_PERI_BUS_AO_PDN,
	DAPC_TYPE_CLUST_BUS_AO_PDN,
	DAPC_TYPE_MAX,
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

struct APC_PERI_DOM_8 {
	unsigned char d0_permission;
	unsigned char d1_permission;
	unsigned char d2_permission;
	unsigned char d3_permission;
	unsigned char d4_permission;
	unsigned char d5_permission;
	unsigned char d6_permission;
	unsigned char d7_permission;
};

struct APC_PERI_DOM_4 {
	unsigned char d0_permission;
	unsigned char d1_permission;
	unsigned char d2_permission;
	unsigned char d3_permission;
};

struct devapc_ao_info {
	uint32_t mod_num;
	uint32_t dom_num;
	uint64_t base;
	uint64_t sec_base;
	const unsigned char *mas_dom;
	const struct APC_PERI_DOM_16 *dev_apc;
};

struct devapc_ao_desc {
	struct devapc_ao_info mas;
	struct devapc_ao_info slv;
};

#define DAPC_PERM_ATTR(PERM_ATTR0, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3, \
		PERM_ATTR4, PERM_ATTR5, PERM_ATTR6, PERM_ATTR7, \
		PERM_ATTR8, PERM_ATTR9, PERM_ATTR10, PERM_ATTR11, \
		PERM_ATTR12, PERM_ATTR13, PERM_ATTR14, PERM_ATTR15) \
	{(unsigned char)PERM_ATTR0, (unsigned char)PERM_ATTR1, \
	(unsigned char)PERM_ATTR2, (unsigned char)PERM_ATTR3, \
	(unsigned char)PERM_ATTR4, (unsigned char)PERM_ATTR5, \
	(unsigned char)PERM_ATTR6, (unsigned char)PERM_ATTR7, \
	(unsigned char)PERM_ATTR8, (unsigned char)PERM_ATTR9, \
	(unsigned char)PERM_ATTR10, (unsigned char)PERM_ATTR11, \
	(unsigned char)PERM_ATTR12, (unsigned char)PERM_ATTR13, \
	(unsigned char)PERM_ATTR14, (unsigned char)PERM_ATTR15}

#define DAPC_TOPS_PERI_BUS_AO_PDN_SYS0_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3, PERM_ATTR4, PERM_ATTR5, \
		PERM_ATTR6, PERM_ATTR7, PERM_ATTR8, PERM_ATTR9, \
		PERM_ATTR10, PERM_ATTR11, PERM_ATTR12, PERM_ATTR13, \
		PERM_ATTR14, PERM_ATTR15) \
	DAPC_PERM_ATTR(PERM_ATTR0, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3, \
		PERM_ATTR4, PERM_ATTR5, PERM_ATTR6, PERM_ATTR7, \
		PERM_ATTR8, PERM_ATTR9, PERM_ATTR10, PERM_ATTR11, \
		PERM_ATTR12, PERM_ATTR13, PERM_ATTR14, PERM_ATTR15)

#define DAPC_TOPS_BUS_AO_PDN_SYS0_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3, PERM_ATTR4, PERM_ATTR5, \
		PERM_ATTR6, PERM_ATTR7, PERM_ATTR8, PERM_ATTR9, \
		PERM_ATTR10, PERM_ATTR11, PERM_ATTR12, PERM_ATTR13, \
		PERM_ATTR14, PERM_ATTR15) \
	DAPC_PERM_ATTR(PERM_ATTR0, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3, \
		PERM_ATTR4, PERM_ATTR5, PERM_ATTR6, PERM_ATTR7, \
		PERM_ATTR8, PERM_ATTR9, PERM_ATTR10, PERM_ATTR11, \
		PERM_ATTR12, PERM_ATTR13, PERM_ATTR14, PERM_ATTR15)

#define DAPC_CLUST_PERI_BUS_AO_PDN_SYS0_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3, PERM_ATTR4, PERM_ATTR5, \
		PERM_ATTR6, PERM_ATTR7, PERM_ATTR8, PERM_ATTR9, \
		PERM_ATTR10, PERM_ATTR11, PERM_ATTR12, PERM_ATTR13, \
		PERM_ATTR14, PERM_ATTR15) \
	DAPC_PERM_ATTR(PERM_ATTR0, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3, \
		PERM_ATTR4, PERM_ATTR5, PERM_ATTR6, PERM_ATTR7, \
		PERM_ATTR8, PERM_ATTR9, PERM_ATTR10, PERM_ATTR11, \
		PERM_ATTR12, PERM_ATTR13, PERM_ATTR14, PERM_ATTR15)

#define DAPC_CLUST_BUS_AO_PDN_SYS0_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3, PERM_ATTR4, PERM_ATTR5, \
		PERM_ATTR6, PERM_ATTR7, PERM_ATTR8, PERM_ATTR9, \
		PERM_ATTR10, PERM_ATTR11, PERM_ATTR12, PERM_ATTR13, \
		PERM_ATTR14, PERM_ATTR15) \
	DAPC_PERM_ATTR(PERM_ATTR0, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3, \
		PERM_ATTR4, PERM_ATTR5, PERM_ATTR6, PERM_ATTR7, \
		PERM_ATTR8, PERM_ATTR9, PERM_ATTR10, PERM_ATTR11, \
		PERM_ATTR12, PERM_ATTR13, PERM_ATTR14, PERM_ATTR15)

#define DAPC_INFRA_AO_SYS0_ATTR(DEV_NAME, PERM_ATTR0, PERM_ATTR1, \
		PERM_ATTR2, PERM_ATTR3, PERM_ATTR4, PERM_ATTR5, \
		PERM_ATTR6, PERM_ATTR7, PERM_ATTR8, PERM_ATTR9, \
		PERM_ATTR10, PERM_ATTR11, PERM_ATTR12, PERM_ATTR13, \
		PERM_ATTR14, PERM_ATTR15) \
	DAPC_PERM_ATTR(PERM_ATTR0, PERM_ATTR1, PERM_ATTR2, PERM_ATTR3, \
		PERM_ATTR4, PERM_ATTR5, PERM_ATTR6, PERM_ATTR7, \
		PERM_ATTR8, PERM_ATTR9, PERM_ATTR10, PERM_ATTR11, \
		PERM_ATTR12, PERM_ATTR13, PERM_ATTR14, PERM_ATTR15)

#define DAPC_MAS_DOM_ATTR(MAS_NAME, DOMAIN_ID) \
	(unsigned char)DOMAIN_ID

/******************************************************************************
 * UTILITY DEFINITION
 ******************************************************************************/
#define devapc_writel(VAL, REG)		mmio_write_32((uintptr_t)REG, VAL)
#define devapc_readl(REG)		mmio_read_32((uintptr_t)REG)

/******************************************************************************/
/* Device APC for TOPS_PERI_BUS_AO_PDN_PDN */
#define DEVAPC_TOPS_PERI_BUS_AO_PDN_PDN_APC_CON \
		(DEVAPC_TOPS_PERI_BUS_AO_PDN_PDN_BASE + 0x0F00)

/******************************************************************************/
/* Device APC for TOPS_PERI_BUS_AO_PDN */
#define DEVAPC_TOPS_PERI_BUS_AO_PDN_APC_CON \
		(DEVAPC_TOPS_PERI_BUS_AO_PDN_BASE + 0x0F00)

#define DEVAPC_TOPS_PERI_BUS_AO_PDN_SYS0_D0_APC_0 \
		(DEVAPC_TOPS_PERI_BUS_AO_PDN_BASE + 0x0000)

/******************************************************************************/
/* Device APC for TOPS_BUS_AO_PDN_PDN */
#define DEVAPC_TOPS_BUS_AO_PDN_PDN_APC_CON \
		(DEVAPC_TOPS_BUS_AO_PDN_PDN_BASE + 0x0F00)

/******************************************************************************/
/* Device APC for TOPS_BUS_AO_PDN */
#define DEVAPC_TOPS_BUS_AO_PDN_APC_CON \
		(DEVAPC_TOPS_BUS_AO_PDN_BASE + 0x0F00)
#define DEVAPC_TOPS_BUS_AO_PDN_MAS_DOM_0 \
		(DEVAPC_TOPS_BUS_AO_PDN_BASE + 0x0900)
#define DEVAPC_TOPS_BUS_AO_PDN_MAS_SEC_0 \
		(DEVAPC_TOPS_BUS_AO_PDN_BASE + 0x0A00)

#define DEVAPC_TOPS_BUS_AO_PDN_SYS0_D0_APC_0 \
		(DEVAPC_TOPS_BUS_AO_PDN_BASE + 0x0000)

/******************************************************************************/
/* Device APC for CLUST_PERI_BUS_AO_PDN_PDN */
#define DEVAPC_CLUST_PERI_BUS_AO_PDN_PDN_APC_CON \
		(DEVAPC_CLUST_PERI_BUS_AO_PDN_PDN_BASE + 0x0F00)

/******************************************************************************/
/* Device APC for CLUST_PERI_BUS_AO_PDN */
#define DEVAPC_CLUST_PERI_BUS_AO_PDN_APC_CON \
		(DEVAPC_CLUST_PERI_BUS_AO_PDN_BASE + 0x0F00)

#define DEVAPC_CLUST_PERI_BUS_AO_PDN_SYS0_D0_APC_0 \
		(DEVAPC_CLUST_PERI_BUS_AO_PDN_BASE + 0x0000)

/******************************************************************************/
/* Device APC for CLUST_BUS_AO_PDN_PDN */
#define DEVAPC_CLUST_BUS_AO_PDN_PDN_APC_CON \
		(DEVAPC_CLUST_BUS_AO_PDN_PDN_BASE + 0x0F00)

/******************************************************************************/
/* Device APC for CLUST_BUS_AO_PDN */
#define DEVAPC_CLUST_BUS_AO_PDN_APC_CON \
		(DEVAPC_CLUST_BUS_AO_PDN_BASE + 0x0F00)
#define DEVAPC_CLUST_BUS_AO_PDN_MAS_DOM_0 \
		(DEVAPC_CLUST_BUS_AO_PDN_BASE + 0x0900)
#define DEVAPC_CLUST_BUS_AO_PDN_MAS_SEC_0 \
		(DEVAPC_CLUST_BUS_AO_PDN_BASE + 0x0A00)

#define DEVAPC_CLUST_BUS_AO_PDN_SYS0_D0_APC_0 \
		(DEVAPC_CLUST_BUS_AO_PDN_BASE + 0x0000)

/******************************************************************************/
/* Device APC for FMEM_AO */
#define DEVAPC_FMEM_AO_APC_CON			(DEVAPC_FMEM_AO_BASE + 0x0F00)
#define DEVAPC_FMEM_AO_MAS_DOM_0		(DEVAPC_FMEM_AO_BASE + 0x0900)
#define DEVAPC_FMEM_AO_MAS_SEC_0		(DEVAPC_FMEM_AO_BASE + 0x0A00)

/******************************************************************************/
/* Device APC for INFRA_AO */
#define DEVAPC_INFRA_AO_APC_CON			(DEVAPC_INFRA_AO_BASE + 0x0F00)
#define DEVAPC_INFRA_AO_MAS_DOM_0		(DEVAPC_INFRA_AO_BASE + 0x0900)
#define DEVAPC_INFRA_AO_MAS_SEC_0		(DEVAPC_INFRA_AO_BASE + 0x0A00)

#define DEVAPC_INFRA_AO_SYS0_D0_APC_0		(DEVAPC_INFRA_AO_BASE + 0x0000)

/******************************************************************************/
/* Device APC for INFRA_PDN */
#define DEVAPC_INFRA_PDN_APC_CON		(DEVAPC_INFRA_PDN_BASE + 0x0F00)

/******************************************************************************/

/******************************************************************************
 * Variable DEFINITION
 ******************************************************************************/
#define MOD_NO_IN_1_DEVAPC			16
#define MAS_NO_IN_1_MAS_DOM			4

#endif /* _DEVAPC_H_ */
