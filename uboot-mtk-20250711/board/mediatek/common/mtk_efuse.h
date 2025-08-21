/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2021 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Alvin Kuo <Alvin.Kuo@mediatek.com>
 */
#ifndef __MTK_EFUSE_H__
#define __MTK_EFUSE_H__

/*********************************************************************************
 *
 *  Returned status of eFuse SMC
 *
 ********************************************************************************/
#define MTK_EFUSE_SUCCESS                                       0x00000000
#define MTK_EFUSE_ERROR_INVALIDE_PARAMTER                       0x00000001
#define MTK_EFUSE_ERROR_INVALIDE_EFUSE_FIELD                    0x00000002
#define MTK_EFUSE_ERROR_EFUSE_FIELD_DISABLED                    0x00000003
#define MTK_EFUSE_ERROR_EFUSE_LEN_EXCEED_BUFFER_LEN             0x00000004
#define MTK_EFUSE_ERROR_READ_EFUSE_FIELD_FAIL                   0x00000005
#define MTK_EFUSE_ERROR_WRITE_EFUSE_FIELD_FAIL                  0x00000006
#define MTK_EFUSE_ERROR_EFUSE_FIELD_SECURE                      0x00000007

/*********************************************************************************
 *
 *  Function ID of eFuse SMC
 *
 ********************************************************************************/
/*
 *  MTK_SIP_EFUSE_GET_LEN - get data length of efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *
 *  return
 *  @r0:        status
 *  @r1:        data length
 */
#define MTK_SIP_EFUSE_GET_LEN           0x82000501

/*
 *  MTK_SIP_EFUSE_WRITE - write efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *  @x2:        addr
 *  @x3:        size
 *
 *  return
 *  @r0:        status
 */
#define MTK_SIP_EFUSE_WRITE             0x82000502

/*
 *  MTK_SIP_EFUSE_READ - read efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *  @x2:        offset
 *
 *  return
 *  @r0:        status
 */
#define MTK_SIP_EFUSE_READ              0x82000503

/*
 *  MTK_SIP_EFUSE_DISABLE - disable efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *
 *  return
 *  @r0:        status
 */
#define MTK_SIP_EFUSE_DISABLE		0x82000504

enum MTK_EFUSE_FIELD {
	EFUSE_AR_EN = 27,
	EFUSE_FW_AR_VER0 = 34,
	EFUSE_FW_AR_VER1
};

int mtk_efuse_read(uint32_t efuse_field,
		   uint8_t *read_buffer,
		   uint32_t read_buffer_len);

int mtk_efuse_write(uint32_t efuse_field,
		    uint8_t *write_buffer,
		    uint32_t write_buffer_len);

int mtk_efuse_disable(uint32_t efuse_field);

#endif /* __MTK_EFUSE_H__ */
