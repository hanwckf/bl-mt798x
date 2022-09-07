/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <plat/common/platform.h>

#include <mtk_efuse.h>

#define MTK_AR_MAX_VER			64
#define MTK_AR_NUM_BL_AR_VER		4

extern const uint32_t bl_ar_ver;

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

	if (count > MTK_AR_MAX_VER)
	{
		ERROR("[%s] count > %d\n",
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

int mtk_ar_get_efuse_bl_ar_ver(uint32_t *efuse_bl_ar_ver)
{
	int ret;
	uint32_t i;
	uint32_t *buffer;
	uint64_t read_buffer[2] = { 0 };

	for (i = 0, buffer = (uint32_t *)read_buffer;
	     i < MTK_AR_NUM_BL_AR_VER;
	     i++, buffer++) {
		ret = mtk_efuse_read(MTK_EFUSE_FIELD_BL_AR_VER0 + i,
				     (uint8_t *)buffer,
				     sizeof(*buffer));
		if (ret) {
			ERROR("[%s] read MTK_EFUSE_FIELD_BL_AR_VER fail (%d)\n",
			      __func__, ret);
			goto error;
		}
	}

	/*
	 *                         bit 0~31              bit 32~63
	 * read_buffer[0] :  EFUSE_BL_AR_VERSION0  EFUSE_BL_AR_VERSION1
	 * read_buffer[1] :  EFUSE_BL_AR_VERSION2  EFUSE_BL_AR_VERSION3
	 * EFUSE_BL_AR_VERSION2 and EFUSE_BL_AR_VERSION3 are used to reduce
	 * fail rate of writing efuse anti-rollback version since efuse
	 * might be wrote unsuccessfully.
	 */
	read_buffer[0] = read_buffer[0] | read_buffer[1];

	VERBOSE("[%s] read_buffer[0] = 0x%llx, read_buffer[1] = 0x%llx\n",
		__func__, read_buffer[0], read_buffer[1]);

	/*
	 * Each bit in read_buffer[0] represent an anti-rollback version,
	 * so we only need to get the far left set bit position.
	 *
	 * bit0  set as high represent the efuse anti-rollback version is 1
	 * bit1  set as high represent the efuse anti-rollback version is 2
	 * ...
	 * bit63 set as high represent the efuse anti-rollback version is 64
	 */
	*efuse_bl_ar_ver = mtk_ar_get_far_left_set_bit_pos(read_buffer[0]);

	VERBOSE("[%s] efuse_bl_ar_ver = %u\n", __func__, *efuse_bl_ar_ver);

	return 0;

error:
	return -1;
}

static int mtk_ar_set_efuse_bl_ar_ver()
{
	int ret;
	uint32_t i;
	uint32_t *buffer;
	uint32_t read_buffer = 0;
	uint64_t write_buffer[2] = { 0 };

	VERBOSE("[%s] bl_ar_ver = %u\n", __func__, bl_ar_ver);

	if (bl_ar_ver > MTK_AR_MAX_VER) {
		ERROR("[%s] Anti-rollback version exceed %d \n",
		      __func__, MTK_AR_MAX_VER);
		goto error;
	}

	/*
	 * Each bit in write_buffer[0] represent an anti-rollback version,
	 * so we need to set these relative bit as high.
	 *
	 * if efuse anti-rollback version is 1, set bit0 as high
	 * if efuse anti-rollback version is 2, set bit0 to bit1 as high
	 * ...
	 * if efuse anti-rollback version is 64, set bit0 to bit63 as high
	 */
	ret = mtk_ar_set_set_bits(write_buffer, bl_ar_ver);
	if (ret)
		goto error;

	/*
	 *                          bit 0~31              bit 32~63
	 * write_buffer[0] :  EFUSE_BL_AR_VERSION0  EFUSE_BL_AR_VERSION1
	 * write_buffer[1] :  EFUSE_BL_AR_VERSION2  EFUSE_BL_AR_VERSION3
	 * EFUSE_BL_AR_VERSION2 and EFUSE_BL_AR_VERSION3 are used to reduce
	 * fail rate of writing efuse anti-rollback version since efuse
	 * might be wrote unsuccessfully.
	 */
	write_buffer[1] = write_buffer[0];

	VERBOSE("[%s] write_buffer[0] = 0x%llx, write_buffer[1] = 0x%llx\n",
		__func__, write_buffer[0], write_buffer[1]);

	for (i = 0, buffer = (uint32_t *)write_buffer;
	     i < MTK_AR_NUM_BL_AR_VER;
	     i++, buffer++) {
		/* no need to write if write data same with fuse data */
		ret = mtk_efuse_read(MTK_EFUSE_FIELD_BL_AR_VER0 + i,
				     (uint8_t *)&read_buffer,
				     sizeof(read_buffer));
		if (ret) {
			ERROR("[%s] read MTK_EFUSE_FIELD_BL_AR_VER fail (%d)\n",
			      __func__, ret);
			goto error;
		}
		if (*buffer == read_buffer)
			continue;

		ret = mtk_efuse_write(MTK_EFUSE_FIELD_BL_AR_VER0 + i,
				      (uint8_t *)buffer,
				      sizeof(*buffer));
		if (ret) {
			ERROR("[%s] write MTK_EFUSE_FIELD_BL_AR_VER fail (%d)\n",
			      __func__, ret);
			goto error;
		}
	}
	return 0;

error:
	return -1;
}

static void mtk_ar_dis_efuse_bl_ar_ver(void)
{
	mtk_efuse_disable(MTK_EFUSE_FIELD_BL_AR_VER0);
	mtk_efuse_disable(MTK_EFUSE_FIELD_BL_AR_VER1);
	mtk_efuse_disable(MTK_EFUSE_FIELD_BL_AR_VER2);
	mtk_efuse_disable(MTK_EFUSE_FIELD_BL_AR_VER3);
}

static int mtk_ar_get_efuse_ar_en(uint32_t *efuse_ar_en)
{
	int ret;

	ret = mtk_efuse_read(MTK_EFUSE_FIELD_AR_EN,
			     (uint8_t *)efuse_ar_en,
			     sizeof(*efuse_ar_en));
	if (ret) {
		ERROR("[%s] read MTK_EFUSE_FIELD_AR_EN fail\n (%d)",
		      __func__, ret);
		goto error;
	}

	return 0;

error:
	return -1;
}

int mtk_ar_update_bl_ar_ver(void)
{
	int ret;
	uint32_t efuse_ar_en;
	uint32_t efuse_bl_ar_ver;

	ret = mtk_ar_get_efuse_ar_en(&efuse_ar_en);
	if (ret)
		goto error;

	ret = mtk_ar_get_efuse_bl_ar_ver(&efuse_bl_ar_ver);
	if (ret)
		goto error;

	VERBOSE("[%s] bl_ar_ver = %u, efuse_bl_ar_ver = %u, efuse_ar_en = %u\n",
		__func__, bl_ar_ver, efuse_bl_ar_ver, efuse_ar_en);

	if (bl_ar_ver > efuse_bl_ar_ver && efuse_ar_en == 1) {
		ret = mtk_ar_set_efuse_bl_ar_ver();
		NOTICE("Updating BL Anti-Rollback Version ... %u -> %u %s\n",
		       efuse_bl_ar_ver, bl_ar_ver, ret == 0 ? "OK" : "FAIL");
		if (ret)
			goto error;
	}

	mtk_ar_dis_efuse_bl_ar_ver();

	return 0;

error:
	return -1;
}

int mtk_ar_check_consis(uint32_t nv_ctr)
{
	VERBOSE("[%s] bl_ar_ver = %u, nv_ctr = %u\n",
		__func__, bl_ar_ver, nv_ctr);

	if (nv_ctr != bl_ar_ver) {
		ERROR("BL Anti-Rollback Version not consistent\n");
		return -1;
	}

	return 0;
}
