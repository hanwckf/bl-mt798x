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

#define MTK_SIP_PWR_ON_MTCMOS		0x82000402
#define MTK_SIP_PWR_OFF_MTCMOS		0x82000403
#define MTK_SIP_PWR_MTCMOS_SUPPORT	0x82000404

#define MTK_SIP_CHECK_FIT_AR_VER	0xC2000520
#define MTK_SIP_UPDATE_EFUSE_AR_VER	0xC2000521

/*******************************************************************************
 * Plat SiP function call records
 ******************************************************************************/
extern struct mtk_sip_call_record mtk_plat_sip_calls[];
extern struct mtk_sip_call_record mtk_plat_sip_calls_from_sec[];
extern const uint32_t mtk_plat_sip_call_num;
extern const uint32_t mtk_plat_sip_call_num_from_sec;

#endif /* PLAT_SIP_CALLS_H */
