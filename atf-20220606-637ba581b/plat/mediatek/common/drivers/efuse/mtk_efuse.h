#ifndef __MTK_EFUSE_H__
#define __MTK_EFUSE_H__

#include <stdbool.h>
#include <lib/mmio.h>

/*********************************************************************************
 *
 *  Returned status of eFuse SMC
 *
 ********************************************************************************/
#define MTK_EFUSE_SUCCESS					0x00000000
#define MTK_EFUSE_ERROR_INVALIDE_PARAMTER			0x00000001
#define MTK_EFUSE_ERROR_INVALIDE_EFUSE_FIELD			0x00000002
#define MTK_EFUSE_ERROR_EFUSE_FIELD_DISABLED			0x00000003
#define MTK_EFUSE_ERROR_EFUSE_LEN_EXCEED_BUFFER_LEN		0x00000004
#define MTK_EFUSE_ERROR_READ_EFUSE_FIELD_FAIL			0x00000005
#define MTK_EFUSE_ERROR_WRITE_EFUSE_FIELD_FAIL			0x00000006

/*********************************************************************************
 *
 *  eFuse info
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

struct mtk_efuse_field_t {
	uint32_t index;
	uint32_t len;
	bool enable;
};

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

#define MTK_EFUSE_PUBK_HASH_LEN			4
#define MTK_EFUSE_SMC_DATA_LEN_MAX		2 * MTK_EFUSE_PUBK_HASH_LEN
#define MTK_EFUSE_SMC_DATA_OFFSET_MAX \
		MTK_EFUSE_PUBK_HASH_INDEX6 * MTK_EFUSE_PUBK_HASH_LEN

uint32_t mtk_efuse_get_len(uint32_t efuse_field,
			   uint32_t *efuse_len_ptr);

uint32_t mtk_efuse_send_data(uint8_t *buffer,
			     uint8_t *data,
			     uint32_t offset,
			     uint32_t len);

uint32_t mtk_efuse_get_data(uint8_t *data,
			    uint8_t *buffer,
			    uint32_t offset,
			    uint32_t len);

uint32_t mtk_efuse_read(uint32_t efuse_field,
			uint8_t *efuse_buffer,
			uint32_t efuse_buffer_len);

uint32_t mtk_efuse_write(uint32_t efuse_field,
			 uint8_t *efuse_buffer,
			 uint32_t efuse_buffer_len);

uint32_t mtk_efuse_disable(uint32_t efuse_field);

#endif /* __MTK_EFUSE_H__ */
