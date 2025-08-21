/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#ifndef MTK_SIP_SVC_H
#define MTK_SIP_SVC_H

#include <stdint.h>
#include <lib/smccc.h>
#include <lib/utils_def.h>

/* SMC function IDs for SiP Service queries */
#define SIP_SVC_CALL_COUNT			U(0x8200ff00)
#define SIP_SVC_UID				U(0x8200ff01)
/* 0x8200ff02 is reserved */
#define SIP_SVC_VERSION				U(0x8200ff03)

/* MediaTek SiP Service Calls version numbers */
#define MTK_SIP_SVC_VERSION_MAJOR		U(0x0)
#define MTK_SIP_SVC_VERSION_MINOR		U(0x1)

/* MediaTek SiP Calls error code */
enum {
	MTK_SIP_E_SUCCESS = 0,
	MTK_SIP_E_INVALID_PARAM = -1,
	MTK_SIP_E_NOT_SUPPORTED = -2,
	MTK_SIP_E_INVALID_RANGE = -3,
	MTK_SIP_E_PERMISSION_DENY = -4,
	MTK_SIP_E_LOCK_FAIL = -5,
};

/* MediaTek SiP Call record */
struct mtk_sip_call_record {
	uint32_t fid;
	rt_svc_handle_t handler;
};

#define MTK_SIP_CALL_RECORD(_fid, _handler) \
	{ .fid = (_fid), .handler = (_handler) }

#endif /* MTK_SIP_SVC_H */
