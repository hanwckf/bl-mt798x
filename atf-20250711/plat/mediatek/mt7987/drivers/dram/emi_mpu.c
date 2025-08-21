// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 */

#include <common/debug.h>
#include <common/bl_common.h>
#include <lib/mmio.h>
#include <emi_mpu.h>
#include <mt7987_def.h>
#include <devapc.h>

static unsigned int region_id;

static const struct emi_mpu_remap_table emi_dom_remap[] = {
	EMI_DOM_REMAP(DOMAIN_0, DOMAIN_0),
	EMI_DOM_REMAP(DOMAIN_1, DOMAIN_1),
	EMI_DOM_REMAP(DOMAIN_2, DOMAIN_2),
	EMI_DOM_REMAP(DOMAIN_3, DOMAIN_3),
	EMI_DOM_REMAP(DOMAIN_4, DOMAIN_4),
	EMI_DOM_REMAP(DOMAIN_5, DOMAIN_5),
	EMI_DOM_REMAP(DOMAIN_6, DOMAIN_6),
	EMI_DOM_REMAP(DOMAIN_7, DOMAIN_7),
	EMI_DOM_REMAP(DOMAIN_8, DOMAIN_8),
	EMI_DOM_REMAP(DOMAIN_9, DOMAIN_9),
	EMI_DOM_REMAP(DOMAIN_10, DOMAIN_10),
	EMI_DOM_REMAP(DOMAIN_11, DOMAIN_11),
	EMI_DOM_REMAP(DOMAIN_12, DOMAIN_12),
	EMI_DOM_REMAP(DOMAIN_13, DOMAIN_13),
	EMI_DOM_REMAP(DOMAIN_14, DOMAIN_14),
	EMI_DOM_REMAP(DOMAIN_15, DOMAIN_15)
};

/*
 * emi_mpu_set_region_protection: protect a region.
 * @start: start address of the region
 * @end: end address of the region
 * @access_permission: EMI MPU access permission
 * Return 0 for success, otherwise negative status code.
 */
static int _emi_mpu_set_protection(
	unsigned long start, unsigned long end,
	unsigned int apc)
{
	unsigned int dgroup;
	unsigned int region;

	region = (start >> 24) & 0xFF;
	start &= 0x00FFFFFF;
	dgroup = (end >> 24) & 0xFF;
	end &= 0x00FFFFFF;

	if  ((region >= EMI_MPU_REGION_NUM) || (dgroup > EMI_MPU_DGROUP_NUM)) {
		WARN("Region:%u or dgroup:%u is wrong!\n", region, dgroup);
		return -1;
	}

	apc &= 0x80FFFFFF;

	if ((start >= DRAM_OFFSET) && (end >= start)) {
		start -= DRAM_OFFSET;
		end -= DRAM_OFFSET;
	} else {
		WARN("start:0x%lx or end:0x%lx address is wrong!\n",
		     start, end);
		return -2;
	}

	mmio_write_32(EMI_MPU_SA(region), start);
	mmio_write_32(EMI_MPU_EA(region), end);
	mmio_write_32(EMI_MPU_APC(region, dgroup), apc);

	return 0;
}

void dump_emi_mpu_regions(void)
{
	unsigned long apc[EMI_MPU_DGROUP_NUM], sa, ea;

	int region, i;

	/* only dump 4 regions */
	for (region = 0; region < 4; ++region) {
		for (i = 0; i < EMI_MPU_DGROUP_NUM; ++i)
			apc[i] = mmio_read_32(EMI_MPU_APC(region, i));
		sa = mmio_read_32(EMI_MPU_SA(region));
		ea = mmio_read_32(EMI_MPU_EA(region));

		INFO("[MPU](Region%d)sa:0x%04lx, ea:0x%04lx\n",
		     region, sa, ea);
		INFO("[MPU](Region%d)apc0:0x%08lx, apc1:0x%08lx\n",
		     region, apc[0], apc[1]);
	}
}

int emi_mpu_set_protection(struct emi_region_info_t *region_info)
{
	unsigned long start, end;
	int i;
	int ret;

	if (region_info->region >= EMI_MPU_REGION_NUM)
		return -1;

	start = (unsigned long)(region_info->start >> EMI_MPU_ALIGN_BITS) |
		(region_info->region << 24);

	for (i = EMI_MPU_DGROUP_NUM - 1; i >= 0; i--) {
		end = (unsigned long)(region_info->end >> EMI_MPU_ALIGN_BITS) |
			(i << 24);
		ret = _emi_mpu_set_protection(start, end, region_info->apc[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static void _set_emi_domain_remap(uint8_t remap_dom_bit, uint8_t domain,
				  uint8_t remap_dom)
{
	uint32_t dom_remap_reg_index;
	uint32_t dom_remap_set_index;
	uintptr_t reg;
	uint32_t clr_bit;
	uint32_t set_bit;

	dom_remap_reg_index = domain * remap_dom_bit / 32;
	dom_remap_set_index = domain * remap_dom_bit % 32;

	clr_bit = GENMASK(remap_dom_bit - 1, 0) << dom_remap_set_index;
	set_bit = (uint32_t)remap_dom << dom_remap_set_index;

	reg = EMI_MPU_DOM_REMAP_TBL0 + dom_remap_reg_index * 4;
	mmio_clrsetbits_32(reg, clr_bit, set_bit);
}

static void set_emi_domain_remap(void)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(emi_dom_remap); i++) {
		_set_emi_domain_remap(EMI_MPU_DOMAIN_BIT, emi_dom_remap[i].dom,
				      emi_dom_remap[i].remap_dom);
	}
}

static void dump_emi_domain_remap(void)
{
	uint32_t i;
	uint32_t reg_num;

	reg_num = (EMI_MPU_DOMAIN_NUM - 1) * EMI_MPU_DOMAIN_BIT / 32;
	for (i = 0; i <= reg_num; i++) {
		VERBOSE("[MPU] DOM_REMAP_TBL%u=%08x\n", i,
			mmio_read_32(EMI_MPU_DOM_REMAP_TBL0 + i * 4));
	}
}

void emi_mpu_init(void)
{
	struct emi_region_info_t region_info;
	int ret;

	set_emi_domain_remap();

	/* TZRAM(BL31) protect address */
	region_info.start = BL31_START - BL31_LOAD_OFFSET;
	region_info.end = (region_info.start + TZRAM_SIZE) - 1;
	region_info.region = region_id++;
	SET_ACCESS_PERMISSION(region_info.apc, 1,
			      FORBID, FORBID, FORBID, FORBID,
			      FORBID, FORBID, FORBID, FORBID,
			      FORBID, FORBID, FORBID, FORBID,
			      FORBID, FORBID, FORBID, SEC_RW);
	ret = emi_mpu_set_protection(&region_info);
	if (ret)
		panic();

#ifdef NEED_BL32
	/* BL32 protect address */
	region_info.start = BL32_TZRAM_BASE;
	region_info.end = (BL32_TZRAM_BASE + BL32_TZRAM_SIZE) - 1;
	region_info.region = region_id++;
	SET_ACCESS_PERMISSION(region_info.apc, 1,
			      FORBID, FORBID, FORBID, FORBID,
			      FORBID, FORBID, FORBID, FORBID,
			      FORBID, FORBID, FORBID, FORBID,
			      FORBID, FORBID, FORBID, SEC_RW);
	ret = emi_mpu_set_protection(&region_info);
	if (ret)
		panic();
#endif

	dump_emi_domain_remap();
	dump_emi_mpu_regions();
}
