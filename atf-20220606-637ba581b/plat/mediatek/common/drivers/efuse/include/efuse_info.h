/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2019. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek Software")
 * have been modified by MediaTek Inc. All revisions are subject to any receiver's
 * applicable license agreements with MediaTek Inc.
 */

#ifndef __EFUSE_INFO_H__
#define __EFUSE_INFO_H__

/**
 * enum efuse_access_type - the access type for efuse
 *
 * @NEVER_WRITE:                TEE app should block all writes to these fuses
 * @UNIT_SPECIFIC_WRITE:        TEE app should only allow writes to these fuses with
 *                              a special per-unit authentication token
 * @SENSITIVE_WRITE:            TEE app should only allow writes to these fuses
 *                              on a given boot before efuse_no_more_sens_write is called
 *                              (from that call until the next boot the writes are disallowed)
 * @SENSITIVE_WRITE_IF_NONZERO: counts as sensitive_write if the current fuse
 *                              value has any non-zero bit; always_write otherwise
 * @ALWAYS_WRITE:               TEE app should always allow writes to these fuses
 *
 */
enum efuse_access_type {
	NEVER_WRITE = 0,
	UNIT_SPECIFIC_WRITE,
	SENSITIVE_WRITE,
	SENSITIVE_WRITE_IF_NONZERO,
	ALWAYS_WRITE
};

/**
 * struct efuse_info - the efuse register information
 *
 * @id:         the efuse index
 * @reg:        the efuse register address
 * @bit_shift:  the bit offset in a efuse register
 * @bit_width:  the width of efuse
 * @access_type:the access type for efuse
 *
 */
struct efuse_info {
	int id;
	unsigned long reg;
	unsigned char bit_shift;
	unsigned int bit_width;
	unsigned char access_type;
};

typedef uint32_t fuse_t;

#define EFUSE_INFO(_id, _reg, _bit_shift, _bit_width, _access_type) {	\
	.id = _id,	\
	.reg = _reg,	\
	.bit_shift = _bit_shift,	\
	.bit_width = _bit_width,	\
	.access_type = _access_type,	\
}

#define REG_BYTE_SIZE		sizeof(fuse_t)
#define REG_BIT_SIZE		(sizeof(fuse_t) << 3)
#define BIT_MASK(x)		((2 << (x - 1)) - 1)
#define BIT_TO_BYTE(x)		(((x % 8) != 0) ? ((x >> 3) + 1) : (x >> 3))
#define EFUSE_BUF_SZ(x)		((x < REG_BYTE_SIZE) ? REG_BYTE_SIZE : x)

/* EFUSEC Control Register status */
#define EFUSE_VALID		(0x00000001)
#define EFUSE_BUSY		(0x00000002)
#define EFUSE_RD		(0x00000004)

#endif
