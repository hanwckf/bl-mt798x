/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
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
 *  MTK_SIP_GET_BL31_REGION - Get real physical memory region of BL31
 *
 *  parameters
 *
 *  return
 *  @r0:	base
 *  @r1:	size
 */
#define MTK_SIP_GET_BL31_REGION			0x82000300

/*
 *  MTK_SIP_GET_BL32_REGION - Get preset physical memory region of BL32
 *
 *  parameters
 *
 *  return
 *  @r0:	base
 *  @r1:	size
 */
#define MTK_SIP_GET_BL32_REGION			0x82000301

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
 *  MTK_SIP_EFUSE_WRITE - write efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *  @x2:	addr
 *  @x3:	size
 *
 *  return
 *  @r0:	status
 */
#define MTK_SIP_EFUSE_WRITE			0xC2000502

/*
 *  MTK_SIP_EFUSE_READ - read efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *  @x2:	offset
 *
 *  return
 *  @r0:	status
 */
#define MTK_SIP_EFUSE_READ			0xC2000503

/*
 *  MTK_SIP_EFUSE_DISABLE - disable efuse field
 *
 *  parameters
 *  @x1:	efuse field
 *
 *  return
 *  @r0:	status
 */
#define MTK_SIP_EFUSE_DISABLE			0xC2000504

/*
 * MTK_SIP_EMERG_MEM_DUMP - Do emergency memory dump thru. ethernet
 *
 * parameters
 * @x1:		reboot after memory dump
 *
 * no return
 */
#define MTK_SIP_EMERG_MEM_DUMP			0xC2000540

/*
 * MTK_SIP_EMERG_MEM_DUMP_CFG - Configure emergency memory dump
 *
 * parameters
 * @x1:		Config operation (0 = Read, 1 = Write)
 * @x2:		Config type
 * @x[3..5]:	Config values (Write only)
 *
 * return
 * @r0:		status
 * @r[1..3]:	Config values (Read only)
 *
 * Config types:
 * 0:		Local IP address (x3 = IPv4 address in network byte order)
 * 1:		Recipient IP address (x3 = IPv4 address in network byte order)
 */
#define MTK_SIP_EMERG_MEM_DUMP_CFG		0xC2000541

/*
 * MTK_SIP_TRNG_GET_RND - Read TRNG
 *
 * return
 * @r0:		status
 * @r1:		random number
 */
#define MTK_SIP_TRNG_GET_RND			0xC2000550

/*
 * MTK_SIP_FWDL_LOAD - Perform firmware download
 *
 * parameters
 * @x1:		FWDL flags
 * @x2:		Firmware data physical address
 * @x3:		Firmware data size
 *
 * return
 * @r0:		status
 */
#define MTK_SIP_FWDL_LOAD			0xC2000560

/*
 * MTK_SIP_READ_NONRST_REG - Read persist registers which will not be reset by
                             watchdog/soft reboot
 *
 * parameters
 * @x1:		Register index (32-bit)
 *
 * return
 * @r0:		Register value (32-bit)
 */
#define MTK_SIP_READ_NONRST_REG			0xC2000570

/*
 * MTK_SIP_WRITE_NONRST_REG - Write persist registers which will not be reset by
                              watchdog/soft reboot
 *
 * parameters
 * @x1:		Register index
 * @x2:		Register value (32-bit)
 *
 * no return
 */
#define MTK_SIP_WRITE_NONRST_REG		0xC2000571

/*
 * MTK_SIP_FW_DEC_SET_IV - Write IV for firmware encryption
 *
 * parameters
 * @x1:		IV physical address
 * @x2:		IV size
 *
 * return
 * @r0:		status
 */
#define MTK_SIP_FW_DEC_SET_IV			0xC2000580

/*
 * MTK_SIP_FW_DEC_SET_KEY - Select key for index
 *
 * parameters
 * @x1:		key index
 *
 * return
 * @r0:		status
 */
#define MTK_SIP_FW_DEC_SET_KEY			0xC2000581

/*
 * MTK_SIP_FW_DEC_IMAGE - Write image for firmware encryption
 *
 * parameters
 * @x1:		image physical address
 * @x2:		image size
 *
 * return
 * @r0:		status
 */
#define MTK_SIP_FW_DEC_IMAGE			0xC2000582

/*
 * MTK_SIP_GET_KEY - Get key to OPTEE
 *
 * parameters
 * @x1:		key index
 * @x1:		key buffer physical address
 * @x2:		key buffer size
 *
 * return
 * @r0:		status
 */
#define MTK_SIP_GET_KEY				0xC2000583

/* ApSoC common SiP function call records */
extern struct mtk_sip_call_record apsoc_common_sip_calls[];
extern struct mtk_sip_call_record apsoc_common_sip_calls_from_sec[];
extern const uint32_t apsoc_common_sip_call_num;
extern const uint32_t apsoc_common_sip_call_num_from_sec;

#endif /* APSOC_SIP_SVC_COMMON_H */
