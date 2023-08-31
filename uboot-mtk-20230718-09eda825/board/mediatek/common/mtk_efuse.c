/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2021 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Alvin Kuo <Alvin.Kuo@mediatek.com>
 */

#include <common.h>
#include <linux/types.h>
#include <linux/arm-smccc.h>

#include "mtk_efuse.h"

static int mtk_efuse_smc(uint32_t smc_fid,
			 uint32_t x1,
			 uint32_t x2,
			 uint32_t x3,
			 uint32_t x4,
			 struct arm_smccc_res *res)
{
	/* SMC64 calling convention is used if in 64bits Uboot */
	if (sizeof(void *) == 8)
		smc_fid |= (0x1 << 30);

	arm_smccc_smc(smc_fid, x1, x2, x3, x4, 0x0, 0x0, 0x0, res);

	return 0;
}

int mtk_efuse_read(uint32_t efuse_field,
		   uint8_t *read_buffer,
		   uint32_t read_buffer_len)
{
	int ret;
	uint32_t idx;
	uint32_t offset;
	uint32_t efuse_len;
	struct arm_smccc_res res;
	uint32_t efuse_data[2] = { 0 };

	/* get efuse length */
	ret = mtk_efuse_smc(MTK_SIP_EFUSE_GET_LEN,
			    efuse_field, 0x0, 0x0, 0x0,
			    &res);
	if (ret < 0)
		return ret;
	else if (res.a0 != MTK_EFUSE_SUCCESS) {
		printf("%s : get efuse length fail (%lu)\n",
		       __func__, res.a0);
		return -1;
	}
	efuse_len = res.a1;

	/* verify buffer */
	if (!read_buffer)
		return -EINVAL;
	if (read_buffer_len < efuse_len)
		return -ENOMEM;

	/* issue efuse read */
	ret = mtk_efuse_smc(MTK_SIP_EFUSE_READ,
			    efuse_field, 0x0, 0x0, 0x0,
			    &res);
	if (ret < 0)
		return ret;
	else if (res.a0 != MTK_EFUSE_SUCCESS) {
		printf("%s : read efuse fail (%lu)\n",
		       __func__, res.a0);
		return -1;
	}

	/* clean read buffer */
	memset(read_buffer, 0x0, read_buffer_len);

	/*
	 * get efuse data
	 * maximum data length in one time SMC is 8 bytes
	 */
	for (offset = 0; offset < efuse_len; offset += 8) {
		ret = mtk_efuse_smc(MTK_SIP_EFUSE_GET_DATA,
				    offset, 8, 0x0, 0x0,
				    &res);
		if (ret < 0)
			return ret;
		else if (res.a0 != MTK_EFUSE_SUCCESS) {
			printf("%s : get efuse data fail (%lu)\n",
			       __func__, res.a0);
			return -1;
		}
		efuse_data[0] = res.a2;
		efuse_data[1] = res.a3;

		for (idx = offset;
		     idx < (offset + 8) && idx < efuse_len;
		     idx++)
		{
			read_buffer[idx] = ((u8 *)efuse_data)[idx - offset];
		}
	}

	return 0;
}

int mtk_efuse_write(uint32_t efuse_field,
		    uint8_t *write_buffer,
		    uint32_t write_buffer_len)
{
	int ret;
	uint32_t idx;
	uint32_t offset;
	uint32_t efuse_len;
	struct arm_smccc_res res;
	uint32_t efuse_data[2] = { 0 };
	uint32_t read_buffer[MTK_EFUSE_PUBK_HASH_INDEX_MAX] = { 0 };

	/* get efuse length */
	ret = mtk_efuse_smc(MTK_SIP_EFUSE_GET_LEN,
			    efuse_field, 0x0, 0x0, 0x0,
			    &res);
	if (ret < 0)
		return ret;
	else if (res.a0 != MTK_EFUSE_SUCCESS) {
		printf("%s : get efuse length fail (%lu)\n",
		       __func__, res.a0);
		return -1;
	}
	efuse_len = res.a1;

	/* verify buffer */
	if (!write_buffer)
		return -EINVAL;
	if (write_buffer_len < efuse_len)
		return -ENOMEM;

	/*
	 * send efuse data
	 * maximum data length in one time SMC is 8 bytes
	 */
	for (offset = 0; offset < efuse_len; offset += 8) {
		memset(efuse_data, 0x0, sizeof(efuse_data));

		for (idx = offset;
		     idx < (offset + 8) && idx < efuse_len;
		     idx++) {
			((u8 *)efuse_data)[idx - offset] = write_buffer[idx];
		}

		ret = mtk_efuse_smc(MTK_SIP_EFUSE_SEND_DATA,
				    offset, idx - offset,
				    efuse_data[0], efuse_data[1],
				    &res);
		if (ret < 0)
			return ret;
		else if (res.a0 != MTK_EFUSE_SUCCESS) {
			printf("%s : send efuse data fail (%lu)\n",
			       __func__, res.a0);
			return -1;
		}
	}

	/*
	 * get efuse data
	 * maximum data length in one time SMC is 8 bytes
	 */
	for (offset = 0; offset < efuse_len; offset += 8) {
		memset(efuse_data, 0x0, sizeof(efuse_data));

		ret = mtk_efuse_smc(MTK_SIP_EFUSE_GET_DATA,
				    offset, 8, 0x0, 0x0,
				    &res);
		if (ret < 0)
			return ret;
		else if (res.a0 != MTK_EFUSE_SUCCESS) {
			printf("%s : get efuse data fail (%lu)\n",
			       __func__, res.a0);
			return -1;
		}
		efuse_data[0] = res.a2;
		efuse_data[1] = res.a3;

		for (idx = offset;
		     idx < (offset + 8) && idx < efuse_len;
		     idx++) {
			((u8 *)read_buffer)[idx] =
				((u8 *)efuse_data)[idx - offset];
		}
	}

	/* verify efuse data */
	if (memcmp(write_buffer, read_buffer, efuse_len)) {
		printf("%s : verify efuse data fail\n", __func__);
		return -1;
	}

	/* issue efuse write */
	ret = mtk_efuse_smc(MTK_SIP_EFUSE_WRITE,
			    efuse_field, 0x0, 0x0, 0x0,
			    &res);
	if (ret < 0)
		return ret;
	else if (res.a0 != MTK_EFUSE_SUCCESS) {
		printf("%s : write efuse fail (%lu)\n", __func__, res.a0);
		return -1;
	}

	return 0;
}

int mtk_efuse_disable(uint32_t efuse_field)
{
	int ret;
	struct arm_smccc_res res;

	ret = mtk_efuse_smc(MTK_SIP_EFUSE_DISABLE,
			    efuse_field, 0x0, 0x0, 0x0,
			    &res);
	if (ret < 0)
		return ret;
	else if (res.a0 != MTK_EFUSE_SUCCESS) {
		printf("%s : disable efuse fail (%lu)\n",
		       __func__, res.a0);
		return -1;
	}

	return 0;
}
