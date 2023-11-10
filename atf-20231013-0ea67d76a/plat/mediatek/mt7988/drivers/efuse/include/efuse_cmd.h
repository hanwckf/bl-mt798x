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

#ifndef __TRUSTZONE_EFUSE_CMD__
#define __TRUSTZONE_EFUSE_CMD__

#include "efuse_atf.h"

#define EFUSE_INDEX_SBC_PUBK0_HASH 8
#define EFUSE_INDEX_SBC_PUBK1_HASH 9
#define EFUSE_INDEX_SBC_PUBK2_HASH 10
#define EFUSE_INDEX_SBC_PUBK3_HASH 11
#define EFUSE_INDEX_AR_VERSION0	   28
#define EFUSE_INDEX_AR_VERSION1	   29
#define EFUSE_INDEX_AR_VERSION2	   30
#define EFUSE_INDEX_AR_VERSION3	   31
#define EFUSE_LENGTH_HASH	   32 /* SBC_PUBKX_HASH length (in byte) */
#define EFUSE_LENGTH_AR		   4 /* AR_VERSIONX length (in byte) */
#define EFUSE_NUM_AR		   4 /* number of AR_VERSIONX */

#define MAX_EFUSE_AR_VERSION 64 /* max value of eFuse Anti-rollback version */

extern int __attribute__((weak))
efuse_read(unsigned int fuse, unsigned char *data, unsigned int len);
extern int __attribute__((weak))
efuse_write(unsigned int fuse, const unsigned char *data, unsigned int len);
extern void __attribute__((weak)) efuse_write_start(void);
extern void __attribute__((weak)) efuse_write_end(void);

#endif /* __TRUSTZONE_EFUSE_CMD__ */
