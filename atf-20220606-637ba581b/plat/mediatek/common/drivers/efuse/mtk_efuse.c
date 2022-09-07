#include <stdio.h>
#include <string.h>
#include <mtk_efuse.h>
#include <efuse_cmd.h>
#include <common/debug.h>

static struct mtk_efuse_field_t
mtk_efuse_field[MTK_EFUSE_FIELD_MAX] = {
#ifdef MTK_EFUSE_FIELD_NORMAL
	[MTK_EFUSE_FIELD_SBC_PUBK0_HASH] = {
		.index = 8,
		.len = 32,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK0_HASH_LOCK] = {
		.index = 16,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_JTAG_DIS] = {
		.index = 25,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_EN] = {
		.index = 26,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_BROM_CMD_DIS] = {
		.index = 33,
		.len = 1,
		.enable = true,
	},
#ifdef MTK_ANTI_ROLLBACK
	[MTK_EFUSE_FIELD_AR_EN] = {
		.index = 27,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_BL_AR_VER0] = {
		.index = 28,
		.len = 4,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_BL_AR_VER1] = {
		.index = 29,
		.len = 4,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_BL_AR_VER2] = {
		.index = 30,
		.len = 4,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_BL_AR_VER3] = {
		.index = 31,
		.len = 4,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_FW_AR_VER0] = {
		.index = 34,
		.len = 4,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_FW_AR_VER1] = {
		.index = 35,
		.len = 4,
		.enable = true,
	},
#endif	/* MTK_ANTI_ROLLBACK */
#endif  /* MTK_EFUSE_FIELD_NORMAL */
/*
 * DO NOT include these efuse fields, unless you know how to use it
 * and want to let these efuse fields can be R/W in normal FW
 */
#ifdef MTK_EFUSE_FIELD_ADVANCED
	[MTK_EFUSE_FIELD_SBC_PUBK1_HASH] = {
		.index = 9,
		.len = 32,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK2_HASH] = {
		.index = 10,
		.len = 32,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK3_HASH] = {
		.index = 11,
		.len = 32,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK1_HASH_LOCK] = {
		.index = 17,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK2_HASH_LOCK] = {
		.index = 18,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK3_HASH_LOCK] = {
		.index = 19,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK0_HASH_DIS] = {
		.index = 21,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK1_HASH_DIS] = {
		.index = 22,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK2_HASH_DIS] = {
		.index = 23,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_SBC_PUBK3_HASH_DIS] = {
		.index = 24,
		.len = 1,
		.enable = true,
	},
	[MTK_EFUSE_FIELD_DAA_EN] = {
		.index = 32,
		.len = 1,
		.enable = true,
	},
#endif  /* MTK_EFUSE_FIELD_ADVANCED */
};

uint32_t mtk_efuse_get_len(uint32_t efuse_field,
			   uint32_t *efuse_len_ptr)
{
	if (!efuse_len_ptr)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	if (efuse_field >= MTK_EFUSE_FIELD_MAX)
		return MTK_EFUSE_ERROR_INVALIDE_EFUSE_FIELD;

	if (mtk_efuse_field[efuse_field].enable == false)
	{
		ERROR("%s : efuse field (%d) was disabled\n",
		      __func__, efuse_field);
		return MTK_EFUSE_ERROR_EFUSE_FIELD_DISABLED;
	}

	*efuse_len_ptr = mtk_efuse_field[efuse_field].len;
	if (0 == *efuse_len_ptr) {
		ERROR("%s : efuse field (%d) was disabled\n",
		      __func__, efuse_field);
		return MTK_EFUSE_ERROR_EFUSE_FIELD_DISABLED;
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_send_data(uint8_t *buffer,
			     uint8_t *data,
			     uint32_t offset,
			     uint32_t len)
{
	uint32_t idx;

	if (offset > MTK_EFUSE_SMC_DATA_OFFSET_MAX ||
	    len > MTK_EFUSE_SMC_DATA_LEN_MAX)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	for (idx = 0; idx < len; idx++) {
		buffer[idx + offset] = data[idx];
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_get_data(uint8_t *data,
			    uint8_t *buffer,
			    uint32_t offset,
			    uint32_t len)
{
	uint32_t idx;

	if (offset > MTK_EFUSE_SMC_DATA_OFFSET_MAX ||
	    len > MTK_EFUSE_SMC_DATA_LEN_MAX)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	for (idx = 0; idx < len; idx++) {
		data[idx] = buffer[idx + offset];
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_read(uint32_t efuse_field,
			uint8_t *efuse_buffer,
			uint32_t efuse_buffer_len)
{
	uint32_t ret;
	uint32_t efuse_len = 0;

	if (!efuse_buffer)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	ret = mtk_efuse_get_len(efuse_field, &efuse_len);
	if (ret != MTK_EFUSE_SUCCESS)
		return ret;

	if (efuse_len > efuse_buffer_len)
		return MTK_EFUSE_ERROR_EFUSE_LEN_EXCEED_BUFFER_LEN;

	memset(efuse_buffer, 0x0, efuse_buffer_len);
	ret = efuse_read(mtk_efuse_field[efuse_field].index,
			 efuse_buffer, efuse_len);
	if (ret) {
		ERROR("%s : read efuse field (%d) fail (%d)\n",
		      __func__, efuse_field, ret);
		return MTK_EFUSE_ERROR_READ_EFUSE_FIELD_FAIL;
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_write(uint32_t efuse_field,
			 uint8_t *efuse_buffer,
			 uint32_t efuse_buffer_len)
{
	uint32_t ret;
	uint32_t efuse_len = 0;

	if (!efuse_buffer)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	ret = mtk_efuse_get_len(efuse_field, &efuse_len);
	if (ret != MTK_EFUSE_SUCCESS)
		return ret;

	if (efuse_len > efuse_buffer_len)
		return MTK_EFUSE_ERROR_EFUSE_LEN_EXCEED_BUFFER_LEN;

	ret = efuse_write(mtk_efuse_field[efuse_field].index,
			  efuse_buffer, efuse_len);
	if (ret) {
		ERROR("%s : write efuse field (%d) fail (%d)\n",
		      __func__, efuse_field, ret);
		return MTK_EFUSE_ERROR_WRITE_EFUSE_FIELD_FAIL;
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_disable(uint32_t efuse_field)
{
	if (efuse_field >= MTK_EFUSE_FIELD_MAX)
		return MTK_EFUSE_ERROR_INVALIDE_EFUSE_FIELD;

	mtk_efuse_field[efuse_field].enable = false;

	return MTK_EFUSE_SUCCESS;
}
