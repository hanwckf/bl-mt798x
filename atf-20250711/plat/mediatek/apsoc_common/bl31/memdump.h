/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef MEM_DUMP_H
#define MEM_DUMP_H

#include <stdint.h>
#include <stdbool.h>

struct mdump_basic_range {
	uintptr_t addr;
	uintptr_t end;
};

struct mdump_range_exclude {
	uint32_t count;
	struct mdump_basic_range r[];
};

struct mdump_range {
	struct mdump_basic_range r;
	bool need_map;
	bool is_device;
	const struct mdump_range_exclude *excludes;
};

/* Memory dump output packet definitions */
struct mdump_basic_header {
	uint32_t magic;
	uint32_t checksum;
	uint32_t version;
	uint32_t assoc_id;
};

struct mdump_control_range {
	uint64_t addr;
	uint64_t end;
};

struct mdump_control_header {
	struct mdump_basic_header bh;
	uint32_t platform;
	uint32_t num_ranges;
	struct mdump_control_range ranges[];
};

struct mdump_range_header {
	struct mdump_basic_header bh;
	uint32_t index;
	uint32_t size;
	uint64_t offset;
	uint8_t data[];	/* Make sure this is 8-byte aligned */
};

struct mdump_range_end_header {
	struct mdump_basic_header bh;
	uint32_t index;
};

struct mdump_context_pstate {
	uint64_t pc;	/* elr_el3 */
	uint64_t cpsr;	/* spsr_el3 */
	uint64_t elr_el1;
	uint64_t elr_el2;
	uint64_t spsr_el1;
	uint64_t spsr_el2;
	uint64_t sp_el0;
	uint64_t sp_el1;
	uint64_t sp_el2;
};

struct mdump_context_sysreg {
	uint64_t sctlr_el3;		/* SPR:0x36100 */
	uint64_t tcr_el3;		/* SPR:0x36202 */
	uint64_t ttbr0_el3;		/* SPR:0x36200 */
	uint64_t mair_el3;		/* SPR:0x36A20 */
	uint64_t scr_el3;		/* SPR:0x36110 */
	uint64_t esr_el3;		/* SPR:0x36520 */
	uint64_t far_el3;		/* SPR:0x36600 */
	uint64_t vbar_el3;		/* SPR:0x36C00 */

	uint64_t sctlr_el2;		/* SPR:0x34100 */
	uint64_t tcr_el2;		/* SPR:0x34202 */
	uint64_t ttbr0_el2;		/* SPR:0x34200 */
	uint64_t mair_el2;		/* SPR:0x34A20 */
	uint64_t esr_el2;		/* SPR:0x34520 */
	uint64_t tpidr_el2;		/* SPR:0x30D02 */
	uint64_t hcr_el2;		/* SPR:0x34110 */
	uint64_t vttbr_el2;		/* SPR:0x34210 */
	uint64_t vtcr_el2;		/* SPR:0x34212 */
	uint64_t far_el2;		/* SPR:0x34600 */
	uint64_t vbar_el2;		/* SPR:0x34C00 */
	uint64_t dbgvcr32_el2;		/* SPR:0x24070 */
	uint64_t hpfar_el2;		/* SPR:0x34604 */
	uint64_t hstr_el2;		/* SPR:0x34113 */
	uint64_t mdcr_el2;		/* SPR:0x34111 */
	uint64_t vmpidr_el2;		/* SPR:0x34005 */
	uint64_t vpidr_el2;		/* SPR:0x34000 */

	uint64_t sctlr_el1;		/* SPR:0x30100 */
	uint64_t tcr_el1;		/* SPR:0x30202 */
	uint64_t ttbr0_el1;		/* SPR:0x30200 */
	uint64_t ttbr1_el1;		/* SPR:0x30201 */
	uint64_t mair_el1;		/* SPR:0x30A20 */
	uint64_t esr_el1;		/* SPR:0x30520 */
	uint64_t tpidr_el1;		/* SPR:0x30D04 */
	uint64_t par_el1;		/* SPR:0x30740 */
	uint64_t far_el1;		/* SPR:0x30600 */
	uint64_t contextidr_el1;	/* SPR:0x30D01 */
	uint64_t vbar_el1;		/* SPR:0x30C00 */

	uint64_t tpidr_el0;		/* SPR:0x33D02 */
	uint64_t tpidrro_el0;		/* SPR:0x33D03 */
};

struct mdump_context_header {
	struct mdump_basic_header bh;
	uint64_t gpr[31];
	struct mdump_context_pstate pstate;
	struct mdump_context_sysreg sysreg;
};

struct mdump_core_data_header {
	struct mdump_basic_header bh;
	uint64_t core_data_pa;
};

struct mdump_end_header {
	struct mdump_basic_header bh;
};

enum mdump_response_type {
	MDUMP_RESP_CONTROL_ACK,
	MDUMP_RESP_RANGE_REXMIT,
	MDUMP_RESP_RANGE_ACK,
	MDUMP_RESP_CONTEXT_ACK,
	MDUMP_RESP_CORE_DATA_ACK,
	MDUMP_RESP_END_ACK,

	__MDUMP_RESP_MAX
};

struct mdump_response_header {
	struct mdump_basic_header bh;
	uint32_t type;
	union {
		struct {
			uint32_t index;
			struct mdump_control_range r;
		} range;
	};
};

#define MDUMP_MAGIC_CONTROL			0x4843444d	/* MDCH */
#define MDUMP_MAGIC_RANGE			0x4452444d	/* MDRD */
#define MDUMP_MAGIC_RANGE_END			0x4552444d	/* MDRE */
#define MDUMP_MAGIC_CONTEXT			0x5443444d	/* MDCT */
#define MDUMP_MAGIC_CORE_DATA			0x4443444d	/* MDCD */
#define MDUMP_MAGIC_END				0x4445444d	/* MDED */
#define MDUMP_MAGIC_RESPONSE			0x5052444d	/* MDRP */

#define MDUMP_PACKET_VERSION			1

#define MDUMP_SRC_PORT				12700
#define MDUMP_DST_PORT				12700

void do_mem_dump(int panic_timeout, uintptr_t core_data_pa, void *handle);
void mem_dump_set_local_ip(uint32_t ip);
uint32_t mem_dump_get_local_ip(void);
void mem_dump_set_dest_ip(uint32_t ip);
uint32_t mem_dump_get_dest_ip(void);

#endif /* MEM_DUMP_H */
