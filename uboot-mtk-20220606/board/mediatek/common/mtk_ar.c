/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <log.h>
#include <common.h>
#include <linux/types.h>
#include <linux/arm-smccc.h>

#include "mtk_efuse.h"

#define MTK_AR_MAX_VER			64
#define MTK_AR_NUM_FW_AR_VER            2

extern int fit_conf_get_fw_ar_ver(const void *fit, int conf_noffset,
				  ulong *fw_ar_ver);

static uint32_t mtk_ar_get_far_left_set_bit_pos(uint64_t num)
{
	uint32_t i;
	uint32_t pos = 0;

	for (i = 1; i <= MTK_AR_MAX_VER && num != 0; i++, num >>= 1) {
		if (num & 0x1)
			pos = i;
	}

	return pos;
}

static int mtk_ar_set_set_bits(uint64_t *num, uint32_t count)
{
	*num = 0;

	if (count > MTK_AR_MAX_VER) {
		printf("[%s] count > %d\n",
		       __func__, MTK_AR_MAX_VER);
		goto error;
	}

	while (count) {
		*num <<= 1;
		*num |= 0x1;
		count--;
	}

	return 0;

error:
	return -1;
}

static void mtk_ar_dis_efuse_fw_ar_ver(void)
{
	mtk_efuse_disable(MTK_EFUSE_FIELD_FW_AR_VER0);
	mtk_efuse_disable(MTK_EFUSE_FIELD_FW_AR_VER1);
}

static int mtk_ar_get_efuse_ar_en(uint32_t *efuse_ar_en)
{
	int ret;

	ret = mtk_efuse_read(MTK_EFUSE_FIELD_AR_EN,
			     (uint8_t *)efuse_ar_en,
			     sizeof(*efuse_ar_en));
	if (ret) {
		printf("[%s] read MTK_EFUSE_FIELD_AR_EN fail\n (%d)",
		       __func__, ret);
		goto error;
	}

	return 0;

error:
	return -1;
}

static int mtk_ar_get_efuse_fw_ar_ver(uint32_t *efuse_fw_ar_ver)
{
	int ret;
	uint32_t i;
	uint32_t *buffer;
	uint64_t read_buffer[1] = { 0 };

	for (i = 0, buffer = (uint32_t *)read_buffer;
	     i < MTK_AR_NUM_FW_AR_VER;
	     i++, buffer++) {
		ret = mtk_efuse_read(MTK_EFUSE_FIELD_FW_AR_VER0 + i,
				     (uint8_t *)buffer,
				     sizeof(*buffer));
		if (ret) {
			printf("[%s] read MTK_EFUSE_FIELD_FW_AR_VER fail (%d)\n",
			       __func__, ret);
			goto error;
		}
	}

	debug("[%s] read_buffer[0] = %llx\n", __func__, read_buffer[0]);

	/*
	 *                         bit 0~31              bit 32~63
	 * read_buffer[0] :  EFUSE_FW_AR_VERSION0  EFUSE_FW_AR_VERSION1
	 */

	/*
	 * Each bit in read_buffer[0] represent an anti-rollback version,
	 * so we only need to get the far left set bit position.
	 *
	 * bit0  set as high represent the efuse anti-rollback version is 1
	 * bit1  set as high represent the efuse anti-rollback version is 2
	 * ...
	 * bit63 set as high represent the efuse anti-rollback version is 64
	 */
	*efuse_fw_ar_ver = mtk_ar_get_far_left_set_bit_pos(read_buffer[0]);

	debug("[%s] efuse_fw_ar_ver = %u\n", __func__, *efuse_fw_ar_ver);

	return 0;

error:
	return -1;
}

static int mtk_ar_set_efuse_fw_ar_ver(uint32_t fw_ar_ver)
{
	int ret;
	uint32_t i;
	uint32_t *buffer;
	uint32_t read_buffer = 0;
	uint64_t write_buffer[1] = { 0 };

	debug("[%s] fw_ar_ver = %u\n", __func__, fw_ar_ver);

	/*
	 * Each bit in write_buffer[0] represent an anti-rollback version,
	 * so we need to set these relative bit as high.
	 *
	 * if efuse anti-rollback version is 1, set bit0 as high
	 * if efuse anti-rollback version is 2, set bit0 to bit1 as high
	 * ...
	 * if efuse anti-rollback version is 64, set bit0 to bit63 as high
	 */
	ret = mtk_ar_set_set_bits(write_buffer, fw_ar_ver);
	if (ret)
		goto error;

	debug("[%s] write_buffer[0] = %llx\n", __func__, write_buffer[0]);

	/*
	 *                          bit 0~31              bit 32~63
	 * write_buffer[0] :  EFUSE_FW_AR_VERSION0  EFUSE_FW_AR_VERSION1
	 */

	for (i = 0, buffer = (uint32_t *)write_buffer;
	     i < MTK_AR_NUM_FW_AR_VER;
	     i++, buffer++) {
		/* no need to write if write data same with fuse data */
		ret = mtk_efuse_read(MTK_EFUSE_FIELD_FW_AR_VER0 + i,
				     (uint8_t *)&read_buffer,
				     sizeof(read_buffer));
		if (ret) {
			printf("[%s] read MTK_EFUSE_FIELD_FW_AR_VER fail (%d)\n",
			       __func__, ret);
			goto error;
		}
		if (*buffer == read_buffer)
			continue;

		ret = mtk_efuse_write(MTK_EFUSE_FIELD_FW_AR_VER0 + i,
				      (uint8_t *)buffer,
				      sizeof(*buffer));
		if (ret) {
			printf("[%s] write MTK_EFUSE_FIELD_FW_AR_VER fail (%d)\n",
			       __func__, ret);
			goto error;
		}
	}

	return 0;

error:
	return -1;
}

int mtk_ar_verify_fw_ar_ver(const void *fit, int conf_noffset,
			    uint32_t *fw_ar_ver)
{
	int ret;
	uint32_t efuse_fw_ar_ver;

	printf("   Verifying FW Anti-Rollback Version ... ");

	ret = fit_conf_get_fw_ar_ver(fit, conf_noffset,
				     (ulong *)fw_ar_ver);
	if (ret) {
		printf("fw_ar_ver:unavailable ");
		goto error;
	} else {
		printf("fw_ar_ver:%u", *fw_ar_ver);
	}

	ret = mtk_ar_get_efuse_fw_ar_ver(&efuse_fw_ar_ver);
	if (ret) {
		printf(",unavailable ");
		goto error;
	}

	if (*fw_ar_ver < efuse_fw_ar_ver) {
		printf("<%u- ", efuse_fw_ar_ver);
		goto error;
	} else if (*fw_ar_ver == efuse_fw_ar_ver) {
		printf("=%u+ ", efuse_fw_ar_ver);
	} else {
		printf(">%u+ ", efuse_fw_ar_ver);
	}

	printf("OK\n");

	return 0;

error:
	printf("FAIL\n");
	return -1;
}

int mtk_ar_update_fw_ar_ver(uint32_t fw_ar_ver)
{
	int ret;
	uint32_t efuse_ar_en;
	uint32_t efuse_fw_ar_ver;

	ret = mtk_ar_get_efuse_ar_en(&efuse_ar_en);
	if (ret)
		goto error;

	ret = mtk_ar_get_efuse_fw_ar_ver(&efuse_fw_ar_ver);
	if (ret)
		goto error;

	debug("[%s] fw_ar_ver = %u, efuse_fw_ar_ver = %u, efuse_ar_en = %u\n",
	      __func__, fw_ar_ver, efuse_fw_ar_ver, efuse_ar_en);

	if (fw_ar_ver > efuse_fw_ar_ver && efuse_ar_en == 1) {
		ret = mtk_ar_set_efuse_fw_ar_ver(fw_ar_ver);
		printf("Updating FW Anti-Rollback Version ... %u -> %u %s\n",
		       efuse_fw_ar_ver, fw_ar_ver, ret == 0 ? "OK" : "FAIL");
		if (ret)
			goto error;
	}

	mtk_ar_dis_efuse_fw_ar_ver();

	return 0;

error:
	return -1;
}
