#ifndef __MTK_EFUSE_H__
#define __MTK_EFUSE_H__

#include <stdbool.h>
#include <lib/mmio.h>
#include <plat_efuse.h>

#define MTK_EFUSE_DRV_VERION					0x2

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
#define MTK_EFUSE_ERROR_EFUSE_FIELD_SECURE			0x00000007

/* use to offset efuse r/w api error code */
#define MTK_EFUSE_ERROR_CODE_OFFSET				0x0000000A

#define MTK_EFUSE_MAX_DATA_LEN					U(32)

uint32_t mtk_efuse_get_len(uint32_t efuse_field,
			   uint32_t *efuse_len_ptr);

uint32_t mtk_efuse_read(uint32_t efuse_field,
			uint8_t *efuse_buffer,
			uint32_t efuse_buffer_len);

uint32_t mtk_efuse_write(uint32_t efuse_field,
			 uint8_t *efuse_buffer,
			 uint32_t efuse_buffer_len);

uint32_t mtk_efuse_disable(uint32_t efuse_field);

uint32_t sip_efuse_read(uint32_t efuse_field, uint32_t ofs, uint32_t *val);

uint32_t sip_efuse_write(uint32_t efuse_field, uintptr_t addr, uint32_t size);

#endif /* __MTK_EFUSE_H__ */
