/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef MT7981_MCUCFG_H
#define MT7981_MCUCFG_H

#include <platform_def.h>
#include <stdint.h>
#include <mt7981_def.h>

struct mt7981_mcucfg_regs {
	uint32_t mp0_ca7l_cache_config;			/* 0x10400000 */
	uint32_t mp0_cpu0_mem_delsel0;			/* 0x10400004 */
	uint32_t mp0_cpu0_mem_delsel1;			/* 0x10400008 */
	uint32_t reserved_0[6];				/* 0x1040000c -- 0x10400020 */
	uint32_t mp0_cache_mem_delsel0;			/* 0x10400024 */
	uint32_t mp0_cache_mem_delsel1;			/* 0x10400028 */
	uint32_t mp0_axi_config;			/* 0x1040002c */
	uint32_t mp0_misc_config0;			/* 0x10400030 */
	uint32_t reserved_1;				/* 0x10400034 */
	uint32_t mp0_misc_config2;			/* 0x10400038 */
	uint32_t mp0_misc_config3;			/* 0x1040003c */
	uint32_t mp0_misc_config4;			/* 0x10400040 */
	uint32_t mp0_misc_config5;			/* 0x10400044 */
	uint32_t mp0_misc_config6;			/* 0x10400048 */
	uint32_t mp0_misc_config7;			/* 0x1040004c */
	uint32_t mp0_misc_config8;			/* 0x10400050 */
	uint32_t mp0_misc_config9;			/* 0x10400054 */
	uint32_t mp0_ca7l_cfg_dis;			/* 0x10400058 */
	uint32_t mp0_ca7l_clken_ctrl;			/* 0x1040005c */
	uint32_t mp0_ca7l_rst_ctrl;			/* 0x10400060 */
	uint32_t mp0_ca7l_misc_config;			/* 0x10400064 */
	uint32_t mp0_ca7l_dbg_pwr_ctrl;			/* 0x10400068 */
	uint32_t mp0_rw_rsvd0;				/* 0x1040006c */
	uint32_t mp0_rw_rsvd1;				/* 0x10400070 */
	uint32_t mp0_ro_rsvd;				/* 0x10400074 */
	uint32_t mp0_mbist_rom_delsel;			/* 0x10400078 */
	uint32_t mp0_l2_cache_parity1_rdata;		/* 0x1040007c */
	uint32_t mp0_l2_cache_parity2_rdata;		/* 0x10400080 */
	uint32_t reserved_2;				/* 0x10400084 */
	uint32_t mp0_rgu_dcm_config;			/* 0x10400088 */
	uint32_t ca53_specific_ctrl;			/* 0x1040008c */
	uint32_t mp0_esr_case;				/* 0x10400090 */
	uint32_t mp0_esr_mask;				/* 0x10400094 */
	uint32_t mp0_esr_trig_en;			/* 0x10400098 */
	uint32_t mp0_mem_eq;				/* 0x1040009c */
	uint32_t ses_cg_en;				/* 0x104000a0 */
	uint32_t reserved_3[37];			/* 0x104000a4 -- 0x10400134 */
	uint32_t mbista_mp0_ocp_result;			/* 0x10400138 */
	uint32_t reserved_4[5];				/* 0x1040013c -- 0x1040014c */
	uint32_t rst_status;				/* 0x10400150 */
	uint32_t dbg_flag;				/* 0x10400154 */
	uint32_t reserved_5[171];			/* 0x10400158 -- 0x10400400 */
	uint32_t mp0_dbg_ctrl;				/* 0x10400404 */
	uint32_t reserved_6;				/* 0x10400408 */
	uint32_t ca7l_ir_mon;				/* 0x1040040c */
	uint32_t reserved_7[32];			/* 0x10400410 -- 0x1040048c */
	uint32_t dfd_ctrl;				/* 0x10400490 */
	uint32_t dfd_cnt_l;				/* 0x10400494 */
	uint32_t dfd_cnt_h;				/* 0x10400498 */
	uint32_t misccfg_rw_rsvd;			/* 0x1040049c */
	uint32_t reserved_8[4];				/* 0x104004a0 -- 0x104004ac */
	uint32_t dvm_dbg_monitor_mp0;			/* 0x104004b0 */
	uint32_t reserved_9[3];				/* 0x104004b4 -- 0x104004bc */
	uint32_t dvm_op_arid_mp0;			/* 0x104004c0 */
	uint32_t reserved_10[53];			/* 0x104004c4 -- 0x10400594 */
	uint32_t misccfg_sec_vio_status0;		/* 0x10400598 */
	uint32_t misccfg_sec_vio_status1;		/* 0x1040059c */
	uint32_t cci_top_if_debug;			/* 0x104005a0 */
	uint32_t cci_m0_if_debug;			/* 0x104005a4 */
	uint32_t cci_m1_if_debug;			/* 0x104005a8 */
	uint32_t cci_m2_if_debug;			/* 0x104005ac */
	uint32_t reserved_11[3];			/* 0x104005b0 -- 0x104005b8 */
	uint32_t cci_s4_if_debug;			/* 0x104005bc */
	uint32_t cci_m0_tra_debug;			/* 0x104005c0 */
	uint32_t cci_m1_tra_debug;			/* 0x104005c4 */
	uint32_t cci_m2_tra_debug;			/* 0x104005c8 */
	uint32_t reserved_12[3];			/* 0x104005cc -- 0x104005d4 */
	uint32_t cci_s4_tra_debug;			/* 0x104005d8 */
	uint32_t cci_tra_dbg_cfg;			/* 0x104005dc */
	uint32_t reserved_13[2];			/* 0x104005e0 -- 0x104005e4 */
	uint32_t gic500_int_mask;			/* 0x104005e8 */
	uint32_t reserved_14;				/* 0x104005ec */
	uint32_t l2c_parity_interrupt_en;		/* 0x104005f0 */
	uint32_t reserved_15[3];			/* 0x104005f4 -- 0x104005fc */
	uint32_t mcusys_config_a;			/* 0x10400600 */
	uint32_t mcusys_config1_a;			/* 0x10400604 */
	uint32_t mcusys_gic_peribase_a;			/* 0x10400608 */
	uint32_t mcusys_pinmux;				/* 0x1040060c */
	uint32_t sec_range0_start;			/* 0x10400610 */
	uint32_t sec_range0_end;			/* 0x10400614 */
	uint32_t sec_range_enable;			/* 0x10400618 */
	uint32_t l2c_mm_base;				/* 0x1040061c */
	uint32_t reserved_16[8];			/* 0x10400620 -- 0x1040063c */
	uint32_t aclken_div;				/* 0x10400640 */
	uint32_t pclken_div;				/* 0x10400644 */
	uint32_t l2c_sram_ctrl;				/* 0x10400648 */
	uint32_t armpll_jit_ctrl;			/* 0x1040064c */
	uint32_t cci_addrmap;				/* 0x10400650 */
	uint32_t cci_config;				/* 0x10400654 */
	uint32_t cci_periphbase;			/* 0x10400658 */
	uint32_t cci_nevntcntovfl;			/* 0x1040065c */
	uint32_t cci_status;				/* 0x10400660 */
	uint32_t reserved_17;				/* 0x10400664 */
	uint32_t mcusys_bus_fabric_dcm_ctrl;		/* 0x10400668 */
	uint32_t mcu_misc_dcm_ctrl;			/* 0x1040066c */
	uint32_t xgpt_ctl;				/* 0x10400670 */
	uint32_t reserved_18[4];			/* 0x10400674 -- 0x10400680 */
	uint32_t mcusys_rw_rsvd0;			/* 0x10400684 */
	uint32_t mcusys_rw_rsvd1;			/* 0x10400688 */
	uint32_t reserved_19[13];			/* 0x1040068c -- 0x104006bc */
	uint32_t gic500_delsel_ctl;			/* 0x104006c0 */
	uint32_t reserved_20[15];			/* 0x104006c4 -- 0x104006fc */
	uint32_t mp_gen_timer_reset_mask_secur_en;	/* 0x10400700 */
	uint32_t mp_gen_timer_reset_mask_0;		/* 0x10400704 */
	uint32_t mp_gen_timer_reset_mask_1;		/* 0x10400708 */
	uint32_t mp_gen_timer_reset_mask_2;		/* 0x1040070c */
	uint32_t mp_gen_timer_reset_mask_3;		/* 0x10400710 */
	uint32_t mp_gen_timer_reset_mask_4;		/* 0x10400714 */
	uint32_t mp_gen_timer_reset_mask_5;		/* 0x10400718 */
	uint32_t mp_gen_timer_reset_mask_6;		/* 0x1040071c */
	uint32_t mp_gen_timer_reset_mask_7;		/* 0x10400720 */
	uint32_t reserved_21[7];			/* 0x10400724 -- 0x1040073c */
	uint32_t mp_cci_adb400_dcm_config;		/* 0x10400740 */
	uint32_t mp_sync_dcm_config;			/* 0x10400744 */
	uint32_t reserved_22;				/* 0x10400748 */
	uint32_t mp_sync_dcm_cluster_config;		/* 0x1040074c */
	uint32_t sw_udi;				/* 0x10400750 */
	uint32_t reserved_23;				/* 0x10400754 */
	uint32_t gic_sync_dcm;				/* 0x10400758 */
	uint32_t reserved_24[3];			/* 0x1040075c -- 0x10400764 */
	uint32_t mcusys_dbg_mon_sel_a;			/* 0x10400768 */
	uint32_t mcusys_dbg_mon;			/* 0x1040076c */
	uint32_t reserved_25[6];			/* 0x10400770 -- 0x10400784 */
	uint32_t mp0_spmc;				/* 0x10400788 */
	uint32_t reserved_26;				/* 0x1040078c */
	uint32_t spmc_mp0_ctl_sram;			/* 0x10400790 */
	uint32_t reserved_27;				/* 0x10400794 */
	uint32_t spmc_mp0_rst;				/* 0x10400798 */
	uint32_t reserved_28[9];			/* 0x1040079c -- 0x104007bc */
	uint32_t bus_pll_divider_cfg;			/* 0x104007c0 */
	uint32_t reserved_29[7];			/* 0x104007c4 -- 0x104007dc */
	uint32_t clusterid_aff1;			/* 0x104007e0 */
	uint32_t clusterid_aff2;			/* 0x104007e4 */
	uint32_t hack_ice_rom_table_access;		/* 0x104007e8 */
	uint32_t reserved_30;				/* 0x104007ec */
	uint32_t l2c_cfg_mp0;				/* 0x104007f0 */
	uint32_t reserved_31[3];			/* 0x104007f4 -- 0x104007fc */
	uint32_t cci_bw_pmu_ctl;			/* 0x10400800 */
	uint32_t cci_bw_pmu_cnt0to1_sel;		/* 0x10400804 */
	uint32_t cci_bw_pmu_cnt2to3_sel;		/* 0x10400808 */
	uint32_t cci_bw_pmu_cnt4to5_sel;		/* 0x1040080c */
	uint32_t cci_bw_pmu_cnt6to7_sel;		/* 0x10400810 */
	uint32_t cci_bw_pmu_cnt0to3_mask;		/* 0x10400814 */
	uint32_t cci_bw_pmu_cnt4to7_mask;		/* 0x10400818 */
	uint32_t cci_bw_pmu_ref_cnt;			/* 0x1040081c */
	uint32_t cci_bw_pmu_acc_cnt0;			/* 0x10400820 */
	uint32_t cci_bw_pmu_acc_cnt1;			/* 0x10400824 */
	uint32_t cci_bw_pmu_acc_cnt2;			/* 0x10400828 */
	uint32_t cci_bw_pmu_acc_cnt3;			/* 0x1040082c */
	uint32_t cci_bw_pmu_acc_cnt4;			/* 0x10400830 */
	uint32_t cci_bw_pmu_acc_cnt5;			/* 0x10400834 */
	uint32_t cci_bw_pmu_acc_cnt6;			/* 0x10400838 */
	uint32_t cci_bw_pmu_acc_cnt7;			/* 0x1040083c */
	uint32_t reserved_32[8];			/* 0x10400840 -- 0x1040085c */
	uint32_t cci_bw_pmu_id_ext_cnt0to3;		/* 0x10400860 */
	uint32_t cci_bw_pmu_id_ext_cnt4to7;		/* 0x10400864 */
	uint32_t cci_bw_pmu_mask_ext_cnt0to3;		/* 0x10400868 */
	uint32_t cci_bw_pmu_mask_ext_cnt4to7;		/* 0x1040086c */
	uint32_t reserved_33[24];			/* 0x10400870 -- 0x104008cc */
	uint32_t mbista_gic_con;			/* 0x104008d0 */
	uint32_t mbista_gic_result;			/* 0x104008d4 */
	uint32_t reserved_34[6];			/* 0x104008d8 -- 0x104008ec */
	uint32_t mbista_rstb;				/* 0x104008f0 */
	uint32_t reserved_35[3];			/* 0x104008f4 -- 0x104008fc */
	uint32_t mp0_hang_monitor_ctrl0;		/* 0x10400900 */
	uint32_t mp0_hang_monitor_ctrl1;		/* 0x10400904 */
	uint32_t reserved_36[62];			/* 0x10400908 -- 0x104009fc */
	uint32_t sec_pol_ctl_en0;			/* 0x10400a00 */
	uint32_t sec_pol_ctl_en1;			/* 0x10400a04 */
	uint32_t sec_pol_ctl_en2;			/* 0x10400a08 */
	uint32_t sec_pol_ctl_en3;			/* 0x10400a0c */
	uint32_t sec_pol_ctl_en4;			/* 0x10400a10 */
	uint32_t sec_pol_ctl_en5;			/* 0x10400a14 */
	uint32_t sec_pol_ctl_en6;			/* 0x10400a18 */
	uint32_t sec_pol_ctl_en7;			/* 0x10400a1c */
	uint32_t sec_pol_ctl_en8;			/* 0x10400a20 */
	uint32_t sec_pol_ctl_en9;			/* 0x10400a24 */
	uint32_t sec_pol_ctl_en10;			/* 0x10400a28 */
	uint32_t sec_pol_ctl_en11;			/* 0x10400a2c */
	uint32_t sec_pol_ctl_en12;			/* 0x10400a30 */
	uint32_t sec_pol_ctl_en13;			/* 0x10400a34 */
	uint32_t sec_pol_ctl_en14;			/* 0x10400a38 */
	uint32_t sec_pol_ctl_en15;			/* 0x10400a3c */
	uint32_t sec_pol_ctl_en16;			/* 0x10400a40 */
	uint32_t sec_pol_ctl_en17;			/* 0x10400a44 */
	uint32_t sec_pol_ctl_en18;			/* 0x10400a48 */
	uint32_t sec_pol_ctl_en19;			/* 0x10400a4c */
	uint32_t reserved_37[12];			/* 0x10400a50 -- 0x10400a7c */
	uint32_t int_pol_ctl0;				/* 0x10400a80 */
	uint32_t int_pol_ctl1;				/* 0x10400a84 */
	uint32_t int_pol_ctl2;				/* 0x10400a88 */
	uint32_t int_pol_ctl3;				/* 0x10400a8c */
	uint32_t int_pol_ctl4;				/* 0x10400a90 */
	uint32_t int_pol_ctl5;				/* 0x10400a94 */
	uint32_t int_pol_ctl6;				/* 0x10400a98 */
	uint32_t int_pol_ctl7;				/* 0x10400a9c */
	uint32_t int_pol_ctl8;				/* 0x10400aa0 */
	uint32_t int_pol_ctl9;				/* 0x10400aa4 */
	uint32_t int_pol_ctl10;				/* 0x10400aa8 */
	uint32_t int_pol_ctl11;				/* 0x10400aac */
	uint32_t int_pol_ctl12;				/* 0x10400ab0 */
	uint32_t int_pol_ctl13;				/* 0x10400ab4 */
	uint32_t int_pol_ctl14;				/* 0x10400ab8 */
	uint32_t int_pol_ctl15;				/* 0x10400abc */
	uint32_t int_pol_ctl16;				/* 0x10400ac0 */
	uint32_t int_pol_ctl17;				/* 0x10400ac4 */
	uint32_t int_pol_ctl18;				/* 0x10400ac8 */
	uint32_t int_pol_ctl19;				/* 0x10400acc */
	uint32_t reserved_38[12];			/* 0x10400ad0 -- 0x10400afc */
	uint32_t dfd_internal_ctl;			/* 0x10400b00 */
	uint32_t dfd_internal_counter;			/* 0x10400b04 */
	uint32_t dfd_internal_pwr_on;			/* 0x10400b08 */
	uint32_t dfd_internal_chain_legth_0;		/* 0x10400b0c */
	uint32_t dfd_internal_shift_clk_ratio;		/* 0x10400b10 */
	uint32_t dfd_internal_counter_return;		/* 0x10400b14 */
	uint32_t dfd_internal_sram_access;		/* 0x10400b18 */
	uint32_t dfd_internal_chain_length_1;		/* 0x10400b1c */
	uint32_t dfd_internal_chain_length_2;		/* 0x10400b20 */
	uint32_t dfd_internal_chain_length_3;		/* 0x10400b24 */
	uint32_t dfd_internal_test_so_0;		/* 0x10400b28 */
	uint32_t dfd_internal_test_so_1;		/* 0x10400b2c */
	uint32_t dfd_internal_num_of_test_so_gp;	/* 0x10400b30 */
	uint32_t dfd_internal_test_so_over_64;		/* 0x10400b34 */
	uint32_t dfd_internal_mask_out;			/* 0x10400b38 */
	uint32_t dfd_internal_sw_ns_trigger;		/* 0x10400b3c */
	uint32_t reserved_39[2];			/* 0x10400b40 -- 0x10400b44 */
	uint32_t dfd_internal_sram_base_addr;		/* 0x10400b48 */
	uint32_t reserved_40[2];			/* 0x10400b4c -- 0x10400b50 */
	uint32_t mp_top_mem_eq;				/* 0x10400b54 */
	uint32_t mcu_all_pwr_on_ctrl;			/* 0x10400b58 */
	uint32_t emi_wfifo;				/* 0x10400b5c */
	uint32_t reserved_41[8];			/* 0x10400b60 -- 0x10400b7c */
	uint32_t bus_dcc_ctrl;				/* 0x10400b80 */
	uint32_t reserved_42[31];			/* 0x10400b84 -- 0x10400bfc */
	uint32_t dfd_v30_ctl_wren;			/* 0x10400c00 */
	uint32_t dfd_v30_base_addr_wren;		/* 0x10400c04 */
	uint32_t bootca_instr1;				/* 0x10400c08 */
	uint32_t bootca_instr2;				/* 0x10400c0c */
	uint32_t bootca_instr3;				/* 0x10400c10 */
	uint32_t bootca_instr4;				/* 0x10400c14 */
	uint32_t bootca_instr5;				/* 0x10400c18 */
	uint32_t bootca_instr6;				/* 0x10400c1c */
	uint32_t bootca_instr7;				/* 0x10400c20 */
	uint32_t bootca_instr8;				/* 0x10400c24 */
	uint32_t bootca_instr9;				/* 0x10400c28 */
	uint32_t bootca_instr10;			/* 0x10400c2c */
	uint32_t bootca_instr11;			/* 0x10400c30 */
	uint32_t bootca_instr12;			/* 0x10400c34 */
	uint32_t bootca_instr13;			/* 0x10400c38 */
	uint32_t bootca_instr14;			/* 0x10400c3c */
	uint32_t bootca_instr15;			/* 0x10400c40 */
	uint32_t bootca_instr16;			/* 0x10400c44 */
	uint32_t bootca_sw_enable;			/* 0x10400c48 */
	uint32_t reserved_43;				/* 0x10400c4c */
	uint32_t ocpb_ctrl;				/* 0x10400c50 */
	uint32_t reserved_44[3];			/* 0x10400c54 -- 0x10400c5c */
	uint32_t core_pc_dbg_mon_sel_ao;		/* 0x10400c60 */
	uint32_t reserved_45[230];			/* 0x10400c64 -- 0x10400ff8 */
	uint32_t mcu_apb_base;				/* 0x10400ffc */
	uint32_t reserved_46[768];			/* 0x10401000 -- 0x10401bfc */
	uint32_t cpusys0_sparkvretcntrl;		/* 0x10401c00 */
	uint32_t cpusys0_sparken;			/* 0x10401c04 */
	uint32_t reserved_47;				/* 0x10401c08 */
	uint32_t cpusys0_cg_dis;			/* 0x10401c0c */
	uint32_t cpusys0_cpu0_counter;			/* 0x10401c10 */
	uint32_t cpusys0_cpu1_counter;			/* 0x10401c14 */
	uint32_t cpusys0_cpu2_counter;			/* 0x10401c18 */
	uint32_t cpusys0_cpu3_counter;			/* 0x10401c1c */
	uint32_t cpusys0_spark_debug_overwrite;		/* 0x10401c20 */
	uint32_t reserved_48[3];			/* 0x10401c24 -- 0x10401c2c */
	uint32_t cpusys0_cpu0_spmc_ctl;			/* 0x10401c30 */
	uint32_t cpusys0_cpu1_spmc_ctl;			/* 0x10401c34 */
	uint32_t cpusys0_cpu2_spmc_ctl;			/* 0x10401c38 */
	uint32_t cpusys0_cpu3_spmc_ctl;			/* 0x10401c3c */
	uint32_t reserved_49[8];			/* 0x10401c40 -- 0x10401c5c */
	uint32_t mp0_sync_dcm_cgavg_ctrl;		/* 0x10401c60 */
	uint32_t mp0_sync_dcm_cgavg_fact;		/* 0x10401c64 */
	uint32_t mp0_sync_dcm_cgavg_rfact;		/* 0x10401c68 */
	uint32_t mp0_sync_dcm_cgavg;			/* 0x10401c6c */
	uint32_t mp0_l2c_parity_clear;			/* 0x10401c70 */

};

static struct mt7981_mcucfg_regs *const mt7981_mcucfg = (void *)MCUCFG_BASE;
void mt7981_mcucfg_selftest(void);

/* cpu boot mode */
#define MP0_CPUCFG_64BIT_SHIFT          12
#define MP0_CPUCFG_64BIT                (U(0xf) << MP0_CPUCFG_64BIT_SHIFT)

#endif /* MT7981_MCUCFG_H */
