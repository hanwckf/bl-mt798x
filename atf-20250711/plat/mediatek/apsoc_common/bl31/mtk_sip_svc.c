// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <tools_share/uuid.h>

#include "mtk_sip_svc.h"
#include "apsoc_sip_svc_common.h"
#include <plat_sip_calls.h>

#define SMC32_PARAM_MASK		0xFFFFFFFF

/* Mediatek SiP Service UUID */
DEFINE_SVC_UUID2(mtk_sip_svc_uid,
	0xa42b58f7, 0x6242, 0x7d4d, 0x80, 0xe5,
	0x8f, 0x95, 0x05, 0x00, 0x0f, 0x3d);

static bool mtk_sip_call_dispatch(const struct mtk_sip_call_record *records,
				  uint32_t count, uintptr_t *retval,
				  uint32_t smc_fid, u_register_t x1,
				  u_register_t x2, u_register_t x3,
				  u_register_t x4, void *cookie, void *handle,
				  u_register_t flags)
{
	uint32_t i;

	for (i = 0; i < count; i++) {
		if (records[i].fid == smc_fid && records[i].handler) {
			*retval = records[i].handler(smc_fid, x1, x2, x3, x4,
						     cookie, handle, flags);
			return true;
		}
	}

	return false;
}

/*
 * This function handles Mediatek defined SiP Calls */
static uintptr_t mediatek_sip_handler(uint32_t smc_fid, u_register_t x1,
				      u_register_t x2, u_register_t x3,
				      u_register_t x4, void *cookie,
				      void *handle, u_register_t flags)
{
	uintptr_t ret;
	uint32_t ns;

	/* if parameter is sent from SMC32. Clean top 32 bits */
	if (GET_SMC_CC(smc_fid) == SMC_32) {
		x1 &= SMC32_PARAM_MASK;
		x2 &= SMC32_PARAM_MASK;
		x3 &= SMC32_PARAM_MASK;
		x4 &= SMC32_PARAM_MASK;
	}

	/* Determine which security state this SMC originated from */
	ns = is_caller_non_secure(flags);
	if (!ns) {
		/* SiP SMC service secure world's call */
		/* Try common services */
		if (mtk_sip_call_dispatch(apsoc_common_sip_calls_from_sec,
					  apsoc_common_sip_call_num_from_sec,
					  &ret, smc_fid, x1, x2, x3, x4,
					  cookie, handle, flags))
			return ret;

		/* Try platform-specific services */
		if (mtk_sip_call_dispatch(mtk_plat_sip_calls_from_sec,
					  mtk_plat_sip_call_num_from_sec,
					  &ret, smc_fid, x1, x2, x3, x4,
					  cookie, handle, flags))
			return ret;
	} else {
		/* SiP SMC service normal world's call */

		/* Try common services */
		if (mtk_sip_call_dispatch(apsoc_common_sip_calls,
					  apsoc_common_sip_call_num,
					  &ret, smc_fid, x1, x2, x3, x4,
					  cookie, handle, flags))
			return ret;

		/* Try platform-specific services */
		if (mtk_sip_call_dispatch(mtk_plat_sip_calls,
					  mtk_plat_sip_call_num,
					  &ret, smc_fid, x1, x2, x3, x4,
					  cookie, handle, flags))
			return ret;
	}

	ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
	SMC_RET1(handle, SMC_UNK);
}

/*
 * This function is responsible for handling all SiP calls from the NS world
 */
static uintptr_t sip_smc_handler(uint32_t smc_fid, u_register_t x1,
				 u_register_t x2, u_register_t x3,
				 u_register_t x4, void *cookie, void *handle,
				 u_register_t flags)
{
	switch (smc_fid) {
	case SIP_SVC_CALL_COUNT:
		/* Return the number of Mediatek SiP Service Calls. */
		SMC_RET1(handle, apsoc_common_sip_call_num + mtk_plat_sip_call_num);

	case SIP_SVC_UID:
		/* Return UID to the caller */
		SMC_UUID_RET(handle, mtk_sip_svc_uid);

	case SIP_SVC_VERSION:
		/* Return the version of current implementation */
		SMC_RET2(handle, MTK_SIP_SVC_VERSION_MAJOR,
			MTK_SIP_SVC_VERSION_MINOR);

	default:
		return mediatek_sip_handler(smc_fid, x1, x2, x3, x4,
			cookie, handle, flags);
	}
}

/* Define a runtime service descriptor for fast SMC calls */
DECLARE_RT_SVC(
	mediatek_sip_svc,
	OEN_SIP_START,
	OEN_SIP_END,
	SMC_TYPE_FAST,
	NULL,
	sip_smc_handler
);
