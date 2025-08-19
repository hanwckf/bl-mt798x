/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APSOC_SIP_SVC_COMMON_H
#define APSOC_SIP_SVC_COMMON_H

#include <stdint.h>

struct mtk_sip_call_record;

/*
 *  MTK_SIP_KERNEL_BOOT_AARCH32 - Boot linux kernel in AArch32 mode
 *
 *  parameters
 *  @x1:	kernel entry point
 *  @x2:	kernel argument r0
 *  @x3:	kernel argument r1
 *  @x4:	kernel mode (0 = 32, others = 64)
 *
 *  no return
 */
#define MTK_SIP_KERNEL_BOOT_AARCH32		0x82000200

/*
 *  MTK_SIP_EFUSE_GET_LEN - get data length of efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *
 *  return
 *  @r0:	status
 *  @r1:	data length
 */
#define MTK_SIP_EFUSE_GET_LEN			0xC2000501

/*
 *  MTK_SIP_EFUSE_SEND_DATA - send data to efuse buffer
 *
 *  parameters
 *  @x1:	data offset, 0 ~ 24 bytes
 *  @x2:	data length, 0 ~ 8 bytes
 *  @x3:	data, bytes 0 to 3
 *  @x4:	data, bytes 4 to 7
 *
 *  return
 *  @r0:	status
 *  @r1:	data length
 */
#define MTK_SIP_EFUSE_SEND_DATA			0xC2000502

/*
 *  MTK_SIP_EFUSE_GET_DATA - get data from efuse buffer
 *
 *  parameters
 *  @x1:	data offset, 0 ~ 24 bytes
 *  @x2:	data length, 0 ~ 8 bytes
 *
 *  return
 *  @r0:	status
 *  @r1:	data length
 *  @r2:	data, bytes 0 to 3
 *  @r3:	data, bytes 4 to 7
 */
#define MTK_SIP_EFUSE_GET_DATA			0xC2000503

/*
 *  MTK_SIP_EFUSE_WRITE - write efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *
 *  return
 *  @r0:	status
 */
#define MTK_SIP_EFUSE_WRITE			0xC2000504

/*
 *  MTK_SIP_EFUSE_READ - read efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *
 *  return
 *  @r0:	status
 */
#define MTK_SIP_EFUSE_READ			0xC2000505

/*
 *  MTK_SIP_EFUSE_DISABLE - disable efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *
 *  return
 *  @r0:	status
 */
#define MTK_SIP_EFUSE_DISABLE			0xC2000506

/*
 * MTK_SIP_FSEK_GET_SHM_CONFIG
 *
 * parameters
 *
 * return
 * @r0:		status
 * @r1:		shm addr
 * @r2:		shm size
 */
#define MTK_SIP_FSEK_GET_SHM_CONFIG		0xC2000520

/*
 * MTK_SIP_FSEK_DECRYPT_RFSK
 *
 * parameters
 *
 * return
 * @r0:		status
 */
#define MTK_SIP_FSEK_DECRYPT_RFSK		0xC2000521

/*
 * MTK_SIP_FSEK_GET_KEY
 *
 * parameters
 * @x1:		key identifier
 *
 * return
 * @r0:		key[63:0]
 * @r1:		key[127:64]
 * @r2:		key[191:128]
 * @r3:		key[255:192]
 */
#define MTK_SIP_FSEK_GET_KEY			0xC2000522

/*
 * MTK_SIP_FSEK_ENCRYPT_ROEK - encrypt roek using SEJ
 *
 * parameters
 * @x1:		roek[63:0]
 * @x2:		roek[127:64]
 * @x3:		roek[191:128]
 * @x4:		roek[255:192]
 *
 * return
 * @r0:		roek_enc[63:0]
 * @r1:		roek_enc[127:64]
 * @r2:		roek_enc[191:128]
 * @r3:		roek_enc[255:192]
 */
#define MTK_SIP_FSEK_ENCRYPT_ROEK		0xC2000530

/*
 * MTK_SIP_EMERG_MEM_DUMP - Do emergency memory dump thru. ethernet
 *
 * parameters
 * @x1:		reboot after memory dump
 *
 * no return
 */
#define MTK_SIP_EMERG_MEM_DUMP			0xC2000540

/* ApSoC common SiP function call records */
extern struct mtk_sip_call_record apsoc_common_sip_calls[];
extern const uint32_t apsoc_common_sip_call_num;

#endif /* APSOC_SIP_SVC_COMMON_H */
