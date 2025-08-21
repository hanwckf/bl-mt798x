/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/runtime_svc.h>
#include <mtcmos.h>
#include <mtk_sip_svc.h>
#include <plat_sip_calls.h>
#include <mtk_efuse.h>
#include <ar_table.h>

static uintptr_t mt7622_sip_mtcmos_pwr_on(uint32_t smc_fid, u_register_t x1,
					  u_register_t x2, u_register_t x3,
					  u_register_t x4, void *cookie,
					  void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtcmos_non_cpu_ctrl(1, (uint32_t)x1);
	if (ret)
		ret = MTK_SIP_E_INVALID_PARAM;

	SMC_RET1(handle, ret);
}

static uintptr_t mt7622_sip_mtcmos_pwr_off(uint32_t smc_fid, u_register_t x1,
					   u_register_t x2, u_register_t x3,
					   u_register_t x4, void *cookie,
					   void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtcmos_non_cpu_ctrl(0, (uint32_t)x1);
	if (ret)
		ret = MTK_SIP_E_INVALID_PARAM;

	SMC_RET1(handle, ret);
}

static uintptr_t mt7622_sip_mtcmos_pwr_support(uint32_t smc_fid,
					       u_register_t x1, u_register_t x2,
					       u_register_t x3, u_register_t x4,
					       void *cookie, void *handle,
					       u_register_t flags)
{
	SMC_RET1(handle, MTK_SIP_E_SUCCESS);
}

static uintptr_t mt7622_sip_ar_check_fip_ver(uint32_t smc_fid, u_register_t x1,
					     u_register_t x2, u_register_t x3,
					     u_register_t x4, void *cookie,
					     void *handle, u_register_t flags)
{
	uint32_t image_fit_ar_ver = (uint32_t)x1;
	uint32_t plat_fit_ar_ver = 0;
	int ret;

	ret = mtk_antirollback_get_fit_ar_ver(&plat_fit_ar_ver);

	if (!ret && image_fit_ar_ver >= plat_fit_ar_ver)
		ret = 1;

	SMC_RET2(handle, ret, plat_fit_ar_ver);
}

static uintptr_t mt7622_sip_ar_update_efuse_ver(uint32_t smc_fid,
						u_register_t x1,
						u_register_t x2,
						u_register_t x3,
						u_register_t x4, void *cookie,
						void *handle,
						u_register_t flags)
{
	int ret;

	ret = mtk_antirollback_update_efuse_ar_ver();
	SMC_RET1(handle, ret);
}

struct mtk_sip_call_record mtk_plat_sip_calls[] = {
	MTK_SIP_CALL_RECORD(MTK_SIP_PWR_ON_MTCMOS, mt7622_sip_mtcmos_pwr_on),
	MTK_SIP_CALL_RECORD(MTK_SIP_PWR_OFF_MTCMOS, mt7622_sip_mtcmos_pwr_off),
	MTK_SIP_CALL_RECORD(MTK_SIP_PWR_MTCMOS_SUPPORT, mt7622_sip_mtcmos_pwr_support),
	MTK_SIP_CALL_RECORD(MTK_SIP_CHECK_FIT_AR_VER, mt7622_sip_ar_check_fip_ver),
	MTK_SIP_CALL_RECORD(MTK_SIP_UPDATE_EFUSE_AR_VER, mt7622_sip_ar_update_efuse_ver),
};

struct mtk_sip_call_record mtk_plat_sip_calls_from_sec[] = {
};

const uint32_t mtk_plat_sip_call_num = ARRAY_SIZE(mtk_plat_sip_calls);
const uint32_t mtk_plat_sip_call_num_from_sec = ARRAY_SIZE(mtk_plat_sip_calls_from_sec);
