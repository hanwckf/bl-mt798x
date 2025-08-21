/* SPDX-License-Identifier:	GPL-2.0+ */
/*
 * Copyright (C) 2021 MediaTek Incorporation. All Rights Reserved.
 *
 * Author: Alvin Kuo <Alvin.Kuo@mediatek.com>
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <linux/io.h>
#include <linux/types.h>
#include <linux/arm-smccc.h>
#include <malloc.h>
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
	uint32_t offset;
	uint32_t efuse_len;
	struct arm_smccc_res res;
	uint32_t val;

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

	/* clean read buffer */
	memset(read_buffer, 0x0, read_buffer_len);

	/* issue efuse read */
	for (offset = 0; offset < efuse_len; offset += 4) {
		ret = mtk_efuse_smc(MTK_SIP_EFUSE_READ,
				    efuse_field, offset, 0x0, 0x0,
				    &res);
		if (ret < 0)
			return ret;
		else if (res.a0 != MTK_EFUSE_SUCCESS) {
			printf("%s : read efuse fail (%lu)\n",
			       __func__, res.a0);
			return -1;
		}
		val = res.a1;

		if (offset + sizeof(val) <= efuse_len)
			memcpy(read_buffer + offset, &val, sizeof(val));
		else
			memcpy(read_buffer + offset, &val, efuse_len - offset);
	}

	return 0;
}

int mtk_efuse_write(uint32_t efuse_field,
		    uint8_t *write_buffer,
		    uint32_t write_buffer_len)
{
	int ret;
	uint32_t efuse_len;
	const uint32_t shm_size = 4096;
	struct arm_smccc_res res;
	void *data;

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

	data = memalign(shm_size, round_up(efuse_len, shm_size));
	if (!data)
		return -ENOMEM;

	memcpy(data, write_buffer, efuse_len);

	/* issue efuse write */
	ret = mtk_efuse_smc(MTK_SIP_EFUSE_WRITE,
			    efuse_field, virt_to_phys(data), efuse_len,
			    0x0, &res);
	if (!ret && res.a0 != MTK_EFUSE_SUCCESS) {
		printf("%s : write efuse fail (%lu)\n", __func__, res.a0);
		ret = -1;
	}

	free(data);

	return ret;
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
