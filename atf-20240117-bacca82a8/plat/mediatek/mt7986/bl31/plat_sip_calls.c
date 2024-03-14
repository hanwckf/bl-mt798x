/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/runtime_svc.h>
#include <mtk_sip_svc.h>
#include <plat_sip_calls.h>

struct mtk_sip_call_record mtk_plat_sip_calls[] = {
	MTK_SIP_CALL_RECORD(0, NULL),
};

const uint32_t mtk_plat_sip_call_num = 0;
