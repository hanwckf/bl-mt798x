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
 *  MTK_SIP_EFUSE_SEND_DATA - send data to efuse buffer
 *
 *  parameters
 *  @x1:        data offset, 0 ~ 24 bytes
 *  @x2:        data length, 0 ~ 8 bytes
 *  @x3:        data, bytes 0 to 3
 *  @x4:        data, bytes 4 to 7
 *
 *  return
 *  @r0:        status
 *  @r1:        data length
 */
#define MTK_SIP_EFUSE_SEND_DATA         0x82000502

/*
 *  MTK_SIP_EFUSE_GET_DATA - get data from efuse buffer
 *
 *  parameters
 *  @x1:        data offset, 0 ~ 24 bytes
 *  @x2:        data length, 0 ~ 8 bytes
 *
 *  return
 *  @r0:        status
 *  @r1:        data length
 *  @r2:        data, bytes 0 to 3
 *  @r3:        data, bytes 4 to 7
 */
#define MTK_SIP_EFUSE_GET_DATA          0x82000503

/*
 *  MTK_SIP_EFUSE_WRITE - write efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *
 *  return
 *  @r0:        status
 */
#define MTK_SIP_EFUSE_WRITE             0x82000504

/*
 *  MTK_SIP_EFUSE_READ - read efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *
 *  return
 *  @r0:        status
 */
#define MTK_SIP_EFUSE_READ              0x82000505

/*
 *  MTK_SIP_EFUSE_DISABLE - disable efuse field
 *
 *  parameters
 *  @x1:        efuse field
 *
 *  return
 *  @r0:        status
 */
#define MTK_SIP_EFUSE_DISABLE		0x82000506

/*********************************************************************************
 *
 *  eFuse info (DO NOT EDIT, it copy from ATF)
 *
 ********************************************************************************/
enum MTK_EFUSE_FIELD {
	MTK_EFUSE_FIELD_SBC_PUBK0_HASH = 0,
	MTK_EFUSE_FIELD_SBC_PUBK1_HASH,
	MTK_EFUSE_FIELD_SBC_PUBK2_HASH,
	MTK_EFUSE_FIELD_SBC_PUBK3_HASH,
	MTK_EFUSE_FIELD_SBC_PUBK0_HASH_LOCK,
	MTK_EFUSE_FIELD_SBC_PUBK1_HASH_LOCK,
	MTK_EFUSE_FIELD_SBC_PUBK2_HASH_LOCK,
	MTK_EFUSE_FIELD_SBC_PUBK3_HASH_LOCK,
	MTK_EFUSE_FIELD_SBC_PUBK0_HASH_DIS,
	MTK_EFUSE_FIELD_SBC_PUBK1_HASH_DIS,
	MTK_EFUSE_FIELD_SBC_PUBK2_HASH_DIS,
	MTK_EFUSE_FIELD_SBC_PUBK3_HASH_DIS,
	MTK_EFUSE_FIELD_JTAG_DIS,
	MTK_EFUSE_FIELD_SBC_EN,
	MTK_EFUSE_FIELD_AR_EN,
	MTK_EFUSE_FIELD_DAA_EN,
	MTK_EFUSE_FIELD_BROM_CMD_DIS,
	MTK_EFUSE_FIELD_BL_AR_VER0,
	MTK_EFUSE_FIELD_BL_AR_VER1,
	MTK_EFUSE_FIELD_BL_AR_VER2,
	MTK_EFUSE_FIELD_BL_AR_VER3,
	MTK_EFUSE_FIELD_FW_AR_VER0,
	MTK_EFUSE_FIELD_FW_AR_VER1,
	__MTK_EFUSE_FIELD_MAX,
};
#define MTK_EFUSE_FIELD_MAX (__MTK_EFUSE_FIELD_MAX)

enum MTK_EFUSE_PUBK_HASH_INDEX {
	MTK_EFUSE_PUBK_HASH_INDEX0 = 0,
	MTK_EFUSE_PUBK_HASH_INDEX1,
	MTK_EFUSE_PUBK_HASH_INDEX2,
	MTK_EFUSE_PUBK_HASH_INDEX3,
	MTK_EFUSE_PUBK_HASH_INDEX4,
	MTK_EFUSE_PUBK_HASH_INDEX5,
	MTK_EFUSE_PUBK_HASH_INDEX6,
	MTK_EFUSE_PUBK_HASH_INDEX7,
	__MTK_EFUSE_PUBK_HASH_INDEX_MAX,
};
#define MTK_EFUSE_PUBK_HASH_INDEX_MAX (__MTK_EFUSE_PUBK_HASH_INDEX_MAX)

int mtk_efuse_read(uint32_t efuse_field,
		   uint8_t *read_buffer,
		   uint32_t read_buffer_len);

int mtk_efuse_write(uint32_t efuse_field,
		    uint8_t *write_buffer,
		    uint32_t write_buffer_len);

int mtk_efuse_disable(uint32_t efuse_field);

#endif /* __MTK_EFUSE_H__ */
