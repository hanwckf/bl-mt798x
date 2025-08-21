/*
 * copyright (c) 2023, mediatek inc. all rights reserved.
 *
 * spdx-license-identifier: bsd-3-clause
 */

#include <common/runtime_svc.h>
#include <mtk_sip_svc.h>
#include <apsoc_sip_svc_common.h>
#include <rng.h>

static uintptr_t mt7981_sip_trng_get_rnd(uint32_t smc_fid, u_register_t x1,
					 u_register_t x2, u_register_t x3,
					 u_register_t x4, void *cookie,
					 void *handle, u_register_t flags)
{
	uint32_t value = 0;
	uintptr_t ret;

	ret = plat_get_rnd(&value);
	SMC_RET2(handle, ret, value);
}

struct mtk_sip_call_record mtk_plat_sip_calls[] = {
	MTK_SIP_CALL_RECORD(MTK_SIP_TRNG_GET_RND, mt7981_sip_trng_get_rnd),
};

struct mtk_sip_call_record mtk_plat_sip_calls_from_sec[] = {
	MTK_SIP_CALL_RECORD(MTK_SIP_TRNG_GET_RND, mt7981_sip_trng_get_rnd),
};

const uint32_t mtk_plat_sip_call_num = ARRAY_SIZE(mtk_plat_sip_calls);
const uint32_t mtk_plat_sip_call_num_from_sec = ARRAY_SIZE(mtk_plat_sip_calls_from_sec);
