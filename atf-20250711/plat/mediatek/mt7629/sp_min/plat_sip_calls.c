/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/runtime_svc.h>
#include <mtk_sip_svc.h>
#include <plat_sip_calls.h>
#include <spmc.h>

static uintptr_t mt7629_sip_mtcmos_pwr_on(uint32_t smc_fid, u_register_t x1,
					  u_register_t x2, u_register_t x3,
					  u_register_t x4, void *cookie,
					  void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtk_spm_non_cpu_ctrl(1, (uint32_t)x1);
	if (ret)
		ret = MTK_SIP_E_INVALID_PARAM;

	SMC_RET1(handle, ret);
}

static uintptr_t mt7629_sip_mtcmos_pwr_off(uint32_t smc_fid, u_register_t x1,
					   u_register_t x2, u_register_t x3,
					   u_register_t x4, void *cookie,
					   void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtk_spm_non_cpu_ctrl(0, (uint32_t)x1);
	if (ret)
		ret = MTK_SIP_E_INVALID_PARAM;

	SMC_RET1(handle, ret);
}

static uintptr_t mt7629_sip_mtcmos_pwr_support(uint32_t smc_fid,
					       u_register_t x1, u_register_t x2,
					       u_register_t x3, u_register_t x4,
					       void *cookie, void *handle,
					       u_register_t flags)
{
	SMC_RET1(handle, MTK_SIP_E_SUCCESS);
}

struct mtk_sip_call_record mtk_plat_sip_calls[] = {
	MTK_SIP_CALL_RECORD(MTK_SIP_PWR_ON_MTCMOS, mt7629_sip_mtcmos_pwr_on),
	MTK_SIP_CALL_RECORD(MTK_SIP_PWR_OFF_MTCMOS, mt7629_sip_mtcmos_pwr_off),
	MTK_SIP_CALL_RECORD(MTK_SIP_PWR_MTCMOS_SUPPORT, mt7629_sip_mtcmos_pwr_support),
};

struct mtk_sip_call_record mtk_plat_sip_calls_from_sec[] = {
};

const uint32_t mtk_plat_sip_call_num = ARRAY_SIZE(mtk_plat_sip_calls);
const uint32_t mtk_plat_sip_call_num_from_sec = ARRAY_SIZE(mtk_plat_sip_calls_from_sec);
