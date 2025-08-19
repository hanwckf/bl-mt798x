/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <pll.h>
#include "pmic_wrap_init.h"

typedef uint32_t (*loop_condition_fp)(uint32_t);

static inline uint32_t wait_for_fsm_vldclr(uint32_t x)
{
	return ((x >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		WACS_FSM_WFVLDCLR;
}

static inline uint32_t wait_for_fsm_idle(uint32_t x)
{
	return ((x >> RDATA_WACS_FSM_SHIFT) & RDATA_WACS_FSM_MASK) !=
		WACS_FSM_IDLE;
}

static inline uint32_t wait_for_state_ready(loop_condition_fp fp,
					    void *wacs_register,
					    uint32_t *read_reg)
{
	uint32_t reg_rdata;

	do {
		reg_rdata = mmio_read_32((uintptr_t)wacs_register);
	} while (fp(reg_rdata));	/* IDLE State */

	if (read_reg)
		*read_reg = reg_rdata;

	return 0;
}

static inline uint32_t wait_for_state_idle(loop_condition_fp fp,
					   void *wacs_register,
					   void *wacs_vldclr_register,
					   uint32_t *read_reg)
{
	uint32_t reg_rdata;

	do {
		reg_rdata = mmio_read_32((uintptr_t)wacs_register);
		/* if last read command timeout,clear vldclr bit
		* read command state machine:FSM_REQ --> wfdle --> WFVLDCLR;
		* write:FSM_REQ --> idle
		*/
		switch (((reg_rdata >> RDATA_WACS_FSM_SHIFT) &
			 RDATA_WACS_FSM_MASK)) {
		case WACS_FSM_WFVLDCLR:
			mmio_write_32((uintptr_t)wacs_vldclr_register, 1);
			ERROR("WACS_FSM = PMIC_WRAP_WACS_VLDCLR\n");
			break;
		case WACS_FSM_WFDLE:
			ERROR("WACS_FSM = WACS_FSM_WFDLE\n");
			break;
		case WACS_FSM_REQ:
			ERROR("WACS_FSM = WACS_FSM_REQ\n");
			break;
		default:
			break;
		}
	} while (fp(reg_rdata));	/* IDLE State */

	if (read_reg)
		*read_reg = reg_rdata;

	return 0;
}

int32_t pwrap_read(uint32_t adr, uint32_t *rdata)
{
	uint32_t reg_rdata;
	uint32_t ret, msb;

	*rdata = 0;
	for (msb = 0; msb < 2; msb++) {
		ret = wait_for_state_idle(wait_for_fsm_idle,
					  &pwrap->wacs2_rdata,
					  &pwrap->wacs2_vldclr,
					  0);
		if (ret) {
			ERROR("wait_for_fsm_idle fail %d\n", ret);
			goto fail;
		}

		mmio_write_32((uintptr_t)&pwrap->wacs2_cmd,
			      (msb << 30) | (adr << 16));

		ret = wait_for_state_ready(wait_for_fsm_vldclr,
					   &pwrap->wacs2_rdata,
					   &reg_rdata);
		if (ret) {
			ERROR("wait_for_fsm_vldclr fail %d\n", ret);
			goto fail;
		}

		*rdata += (((reg_rdata >> RDATA_WACS_RDATA_SHIFT) &
			   RDATA_WACS_RDATA_MASK) << (16 * msb));

		mmio_write_32((uintptr_t)&pwrap->wacs2_vldclr, 1);
	}
fail:
	return ret;
}

int32_t pwrap_write(uint32_t adr, uint32_t wdata)
{
	uint32_t rdata;
	uint32_t ret;
	uint32_t msb;

	for (msb = 0; msb < 2; msb++) {
		ret = wait_for_state_idle(wait_for_fsm_idle,
					  &pwrap->wacs2_rdata,
					  &pwrap->wacs2_vldclr,
					  0);
		if (ret) {
			ERROR("wait_for_fsm_idle fail %d\n", ret);
			goto fail;
		}

		mmio_write_32((uintptr_t)&pwrap->wacs2_cmd,
			      BIT(31) | (msb << 30) | (adr << 16) |
			      ((wdata >> (msb * 16)) & 0xffff));

		/*
		 * The pwrap_read operation is the requirement of hardware used
		 * for the synchronization between two successive 16-bit
		 * pwrap_writel operations composing one 32-bit bus writing.
		 * Otherwise, we'll find the result fails on the lower 16-bit
		 * pwrap writing.
		 */
		if (!msb)
			pwrap_read(adr, &rdata);
	}
fail:
	return ret;
}

void mtk_pwrap_init(void)
{
	/* Toggle soft reset */
	mmio_write_32(INFRACFG_AO_BASE + INFRA_GLOBALCON_RST0, PWRAP_SOFT_RST);
	mmio_write_32(INFRACFG_AO_BASE + INFRA_GLOBALCON_RST1, PWRAP_SOFT_RST);

	/* Enable DCM */
	mmio_write_32((uintptr_t)&pwrap->dcm_en, 0x3);
	mmio_write_32((uintptr_t)&pwrap->dcm_dbc_prd, 0x0);

	/* Enable PMIC_WRAP */
	mmio_write_32((uintptr_t)&pwrap->wrap_en, 0x1);
	mmio_write_32((uintptr_t)&pwrap->hiprio_arb_en, 0xff);
	mmio_write_32((uintptr_t)&pwrap->wacs2_en, 0x1);
	mmio_write_32((uintptr_t)&pwrap->wacs0_en, 0x1);
	mmio_write_32((uintptr_t)&pwrap->wacs1_en, 0x1);

	mmio_write_32((uintptr_t)&pwrap->staupd_prd, 0x0);
	mmio_write_32((uintptr_t)&pwrap->wdt_unit, 0xf);
	mmio_write_32((uintptr_t)&pwrap->wdt_src_en, 0xffffffff);
	mmio_write_32((uintptr_t)&pwrap->timer_en, 0x1);
	mmio_write_32((uintptr_t)&pwrap->int_en, 0x7fffffff);

	/* Initialization Done */
	mmio_write_32((uintptr_t)&pwrap->init_done0, 0x1);
	mmio_write_32((uintptr_t)&pwrap->init_done1, 0x1);
	mmio_write_32((uintptr_t)&pwrap->init_done2, 0x1);

	/* 2 wire SPI master enable */
	mmio_write_32((uintptr_t)&pwrap->spi2_ctrl, PWRAP_SPI_MASTER_EN);
}
