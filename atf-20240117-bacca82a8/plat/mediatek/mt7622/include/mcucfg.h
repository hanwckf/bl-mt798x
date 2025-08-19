/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MCUCFG_H
#define MCUCFG_H

#include <stdint.h>

#include <mt7622_def.h>

struct mt7622_mcucfg_regs {
	uint32_t mp0_ca7l_cache_config;
	struct {
		uint32_t mem_delsel0;
		uint32_t mem_delsel1;
	} mp0_cpu[4];
	uint32_t mp0_cache_mem_delsel0;
	uint32_t mp0_cache_mem_delsel1;
	uint32_t mp0_axi_config;
	uint32_t mp0_misc_config[10];
	uint32_t mp0_ca7l_cfg_dis;
	uint32_t mp0_ca7l_clken_ctrl;
	uint32_t mp0_ca7l_rst_ctrl;
	uint32_t mp0_ca7l_misc_config;
	uint32_t mp0_ca7l_dbg_pwr_ctrl;
	uint32_t mp0_rw_rsvd0;
	uint32_t mp0_rw_rsvd1;
	uint32_t mp0_ro_rsvd;
	uint32_t reserved0_0[226];
	uint32_t mp0_rst_status;		/* 0x400 */
	uint32_t mp0_dbg_ctrl;
	uint32_t mp0_dbg_flag;
	uint32_t mp0_ca7l_ir_mon;
	struct {
		uint32_t pc_lw;
		uint32_t pc_hw;
		uint32_t fp_arch32;
		uint32_t sp_arch32;
		uint32_t fp_arch64_lw;
		uint32_t fp_arch64_hw;
		uint32_t sp_arch64_lw;
		uint32_t sp_arch64_hw;
	} mp0_dbg_core[4];
	uint32_t dfd_ctrl;
	uint32_t dfd_cnt_l;
	uint32_t dfd_cnt_h;
	uint32_t misccfg_ro_rsvd;		/* 0x49c */
	uint32_t reserved1[60];
	uint32_t mcusys_dbg_mon_sel_a;
	uint32_t mcusys_dbg_mon;
	uint32_t misccfg_sec_vio_status0;
	uint32_t misccfg_sec_vio_status1;
	uint32_t cci_top_if_debug;
	uint32_t cci_m0_if_debug;
	uint32_t reserved2;
	uint32_t cci_m2_if_debug;
	uint32_t cci_s2_if_debug;
	uint32_t reserved3[2];
	uint32_t cci_s4_if_debug;
	uint32_t cci_m0_tra_debug;
	uint32_t reserved4;
	uint32_t cci_m2_tra_debug;
	uint32_t reserved5;
	uint32_t cci_s2_tra_debug;
	uint32_t reserved6;
	uint32_t cci_s4_tra_debug;
	uint32_t cci_tra_dbg_cfg;
	uint32_t reserved7[8];
	uint32_t mcusys_config_a;		/* 0x600 */
	uint32_t mcusys_config1_a;
	uint32_t mcusys_gic_peribase_a;
	uint32_t reserved8;
	uint32_t sec_range0_start;		/* 0x610 */
	uint32_t sec_range0_end;
	uint32_t sec_range_enable;
	uint32_t reserved9;
	uint32_t int_pol_ctl[8];		/* 0x620 */
	uint32_t aclken_div;			/* 0x640 */
	uint32_t pclken_div;
	uint32_t l2c_sram_ctrl;
	uint32_t armpll_jit_ctrl;
	uint32_t cci_addrmap;			/* 0x650 */
	uint32_t cci_config;
	uint32_t cci_periphbase;
	uint32_t cci_nevntcntovfl;
	uint32_t cci_clk_ctrl;			/* 0x660 */
	uint32_t cci_acel_s1_ctrl;
	uint32_t bus_fabric_dcm_ctrl;
	uint32_t reserved10;
	uint32_t xgpt_ctl;			/* 0x670 */
	uint32_t xgpt_idx;
	uint32_t ptpod2_ctl0;
	uint32_t ptpod2_ctl1;
	uint32_t mcusys_revid;
	uint32_t mcusys_rw_rsvd0;
	uint32_t mcusys_rw_rsvd1;
	uint32_t reserved11[29];
	uint32_t gen_timer_rst_msk_secur_en;
	uint32_t gen_timer_rst_msk[4];
	uint32_t reserved12[11];
	uint32_t mp_sync_dcm_config;
};

static struct mt7622_mcucfg_regs *const mt7622_mcucfg = (void *)MCUCFG_BASE;

/* cpu boot mode */
#define	MP0_CPUCFG_64BIT_SHIFT		12
#define	MP0_CPUCFG_64BIT		(U(0xf) << MP0_CPUCFG_64BIT_SHIFT)

/* L2C SRAM interface and MCU clock divider */
#define MCU_BUS_DIV2			0x12

/* scu related */
enum {
	MP0_ACINACTM_SHIFT = 4,
	MP0_ACINACTM = 1 << MP0_ACINACTM_SHIFT
};

#endif /* MCUCFG_H */
