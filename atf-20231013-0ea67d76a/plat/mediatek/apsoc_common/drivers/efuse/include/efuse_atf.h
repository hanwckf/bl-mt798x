/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2016. All rights reserved.
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

#ifndef __TRUSTZONE_TA_EFUSE__
#define __TRUSTZONE_TA_EFUSE__

typedef unsigned char u8;
typedef unsigned int u32;

/**
 * enum efuse_cmd - the efuse service call command
 *
 * @TZCMD_EFUSE_READ:              read command
 * @TZCMD_EFUSE_WRITE:             write command
 * @TZCMD_EFUSE_NOMORE_SENS_WRITE: if set, no more sensitive efuse data
 *                                 can be write
 * @TZCMD_EFUSE_WRITE_START:       start to write efuse
 * @TZCMD_EFUSE_WRITE_END:         end to write efuse
 *
 */
enum efuse_cmd {
	TZCMD_EFUSE_READ = 0,
	TZCMD_EFUSE_WRITE
};

/* efuse status code */
#define TZ_EFUSE_OK			0	/* eFuse is ok */
#define TZ_EFUSE_BUSY			1	/* eFuse is busy */
#define TZ_EFUSE_INVAL			2	/* input parameters is invalid */
#define TZ_EFUSE_LEN			3	/* incorrect length of eFuse value */
#define TZ_EFUSE_ZERO_VAL		4	/* can't write a zero efuse value */
#define TZ_EFUSE_ACCESS_TYPE		5	/* incorrect eFuse access type */
#define TZ_EFUSE_BLOWN			6	/* the eFuse has been blown */
#define TZ_EFUSE_READ_AFTER_WRITE	7	/* check eFuse read failed after eFuse is written */
#define TZ_EFUSE_BUSY_LOCK		8	/* eFuse is locking */
#define TZ_EFUSE_RES			9	/* resource problem duing eFuse operation */
#define TZ_EFUSE_VAL_OUT_SPEC		10	/* input eFuse value is out of sepcification */

#endif /* __TRUSTZONE_TA_EFUSE__ */
