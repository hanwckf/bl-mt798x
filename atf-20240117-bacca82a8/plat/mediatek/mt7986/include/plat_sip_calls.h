/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PLAT_SIP_CALLS_H
#define PLAT_SIP_CALLS_H

#include <stdint.h>

struct mtk_sip_call_record;

/*******************************************************************************
 * Plat SiP function constants
 ******************************************************************************/

/*******************************************************************************
 * Plat SiP function call records
 ******************************************************************************/
extern struct mtk_sip_call_record mtk_plat_sip_calls[];
extern const uint32_t mtk_plat_sip_call_num;

#endif /* PLAT_SIP_CALLS_H */
