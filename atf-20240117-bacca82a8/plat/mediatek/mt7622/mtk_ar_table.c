/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <plat/common/platform.h>
#include <common/debug.h>
#include <ar_table.h>
#include <efuse_cmd.h>
#include <mtk_efuse.h>

#define EFUSE_INDEX_AR_VERSION0			28
#define EFUSE_INDEX_AR_VERSION1			29
#define EFUSE_INDEX_AR_VERSION2			30
#define EFUSE_INDEX_AR_VERSION3			31
#define EFUSE_LENGTH_AR				4
#define EFUSE_NUM_AR				4
#define MAX_EFUSE_AR_VERSION			64

#ifdef ANTI_ROLLBACK
extern const uint32_t mtk_ar_table_num_entry;
extern struct ar_table_entry mtk_ar_table[];
#else
const uint32_t mtk_ar_table_num_entry = 0;
struct ar_table_entry *mtk_ar_table = NULL;
#endif

static int mtk_antirollback_check_mtk_ar_table_parms(void)
{

	if (mtk_ar_table == NULL) {
		ERROR("[%s] check MTK_AR_TABLE fail, "
		      "mtk_ar_table is null\n", __func__);
		goto error;
	}

	if (mtk_ar_table_num_entry == 0 ||
	    mtk_ar_table_num_entry > MAX_EFUSE_AR_VERSION + 1) {
		ERROR("[%s] check MTK_AR_TABLE_NUM_ENTRY fail, "
		      "invalid number of mtk_ar_table entry\n", __func__);
		goto error;
	}

	return 0;

error:
	return -1;
}

static uint32_t mtk_antirollback_get_far_left_set_bit_pos(uint64_t num)
{
	uint32_t i, pos = 0;

	for (i = 1; i <= MAX_EFUSE_AR_VERSION && num != 0; i++, num >>= 1) {
		if (num & 0x1)
			pos = i;
	}

	return pos;
}

static int mtk_antirollback_read_efuse_ar_vers(uint32_t *buffer, size_t buffer_size)
{
	int ret;
	unsigned int i;

	if (buffer_size / EFUSE_LENGTH_AR < EFUSE_NUM_AR)
		goto error;

	for (i = 0; i < EFUSE_NUM_AR; i++, buffer++) {
		ret = efuse_read(EFUSE_INDEX_AR_VERSION0 + i,
				 (uint8_t *)buffer,
				 EFUSE_LENGTH_AR);
		if (ret)
			goto error;
	}

	return 0;

error:
	return -1;
}

static void mtk_antirollback_set_set_bits(uint64_t *num, uint32_t count)
{
	*num = 0;

	while (count) {
		*num <<= 1;
		*num |= 0x1;
		count--;
	}

	return;
}

static int mtk_antirollback_write_efuse_ar_vers(uint32_t *buffer, size_t buffer_size)
{
	int ret;
	unsigned int i;

	if (buffer_size / EFUSE_LENGTH_AR < EFUSE_NUM_AR)
		goto error;

	for (i = 0; i < EFUSE_NUM_AR; i++, buffer++) {
		ret = efuse_write(EFUSE_INDEX_AR_VERSION0 + i,
				  (uint8_t *)buffer,
				  EFUSE_LENGTH_AR);
		if (ret && ret != -TZ_EFUSE_ZERO_VAL && ret != -TZ_EFUSE_BLOWN)
		{
			ERROR("[%s] eFuse write fail (ret = %d)\n",
			       __func__, ret);
			goto error;
		}
	}

	return 0;

error:
	return -1;
}

static int mtk_antirollback_set_efuse_ar_ver(uint32_t ar_ver)
{
	int ret;
	uint64_t write_buffer[2] = { 0 };

	/*
	 * Each bit in write_buffer[0] represent an anti-rollback version,
	 * so we need to set these relative bit as high.
	 *
	 * if efuse anti-rollback version is 1, set bit0 as high
	 * if efuse anti-rollback version is 2, set bit0 to bit1 as high
	 * ...
	 * if efuse anti-rollback version is 64, set bit0 to bit63 as high
	 */
	mtk_antirollback_set_set_bits(write_buffer, ar_ver);

	/*
	 *                      bit 0~31           bit 32~63
	 * write_buffer[0] :  EFUSE_AR_VERSION0  EFUSE_AR_VERSION1
	 * write_buffer[1] :  EFUSE_AR_VERSION2  EFUSE_AR_VERSION3
	 * EFUSE_AR_VERSION2 and EFUSE_AR_VERSION3 are used to reduce
	 * fail rate of writing efuse anti-rollback version since efuse
	 * might be wrote unsuccessfully.
	 */
	write_buffer[1] = write_buffer[0];

	ret = mtk_antirollback_write_efuse_ar_vers((uint32_t *)write_buffer,
						   sizeof(write_buffer));
	if (ret) {
		ERROR("[%s] write EFUSE_AR_VERS fail\n", __func__);
		goto error;
	}

	return 0;

error:
	return -1;
}

static int mtk_antirollback_get_efuse_ar_ver(uint32_t *efuse_ar_ver)
{
	int ret;
	uint64_t read_buffer[2] = { 0 };

	ret = mtk_antirollback_read_efuse_ar_vers((uint32_t *)read_buffer,
						  sizeof(read_buffer));
	if (ret) {
		ERROR("[%s] read EFUSE_AR_VERS fail\n", __func__);
		goto error;
	}

	/*
	 *                      bit 0~31           bit 32~63
	 * read_buffer[0] :  EFUSE_AR_VERSION0  EFUSE_AR_VERSION1
	 * read_buffer[1] :  EFUSE_AR_VERSION2  EFUSE_AR_VERSION3
	 * EFUSE_AR_VERSION2 and EFUSE_AR_VERSION3 are used to reduce
	 * fail rate of writing efuse anti-rollback version since efuse
	 * might be wrote unsuccessfully.
	 */
	read_buffer[0] = read_buffer[0] | read_buffer[1];

	/*
	 * Each bit in read_buffer[0] represent an anti-rollback version,
	 * so we only need to get the far left set bit position.
	 *
	 * bit0  set as high represent the efuse anti-rollback version is 1
	 * bit1  set as high represent the efuse anti-rollback version is 2
	 * ...
	 * bit63 set as high represent the efuse anti-rollback version is 64
	 */
	*efuse_ar_ver = mtk_antirollback_get_far_left_set_bit_pos(read_buffer[0]);

	return 0;

error:
	return -1;
}

int mtk_antirollback_get_bl2_ar_ver(void)
{
	int ret;
	uint32_t efuse_ar_ver;
	uint32_t latest_efuse_ar_ver;

	ret = mtk_antirollback_check_mtk_ar_table_parms();
	if (ret)
		goto error;

	ret = mtk_antirollback_get_efuse_ar_ver(&efuse_ar_ver);
	if (ret) {
		ERROR("[%s] get EFUSE_AR_VER fail\n", __func__);
		goto error;
	}
	latest_efuse_ar_ver = mtk_ar_table_num_entry - 1;

	if (efuse_ar_ver > latest_efuse_ar_ver) {
		ERROR("[%s] check EFUSE_AR_VER fail, "
		      "can't greater than LATEST_EFUSE_AR_VER\n", __func__);
		goto error;
	}

	return mtk_ar_table[efuse_ar_ver].bl2_ar_ver;

error:
	return -1;
}

int mtk_antirollback_get_tfw_ar_ver(void)
{
	int ret;
	uint32_t efuse_ar_ver;
	uint32_t latest_efuse_ar_ver;

	ret = mtk_antirollback_check_mtk_ar_table_parms();
	if (ret)
		goto error;

	ret = mtk_antirollback_get_efuse_ar_ver(&efuse_ar_ver);
	if (ret) {
		ERROR("[%s] get EFUSE_AR_VER fail\n", __func__);
		goto error;
	}
	latest_efuse_ar_ver = mtk_ar_table_num_entry - 1;

	if (efuse_ar_ver > latest_efuse_ar_ver) {
		ERROR("[%s] check EFUSE_AR_VER fail, "
		      "can't greater than LATEST_EFUSE_AR_VER\n", __func__);
		goto error;
	}

	return mtk_ar_table[efuse_ar_ver].tfw_ar_ver;

error:
	return -1;
}

int mtk_antirollback_get_ntfw_ar_ver(void)
{
	int ret;
	uint32_t efuse_ar_ver;
	uint32_t latest_efuse_ar_ver;

	ret = mtk_antirollback_check_mtk_ar_table_parms();
	if (ret)
		goto error;

	ret = mtk_antirollback_get_efuse_ar_ver(&efuse_ar_ver);
	if (ret) {
		ERROR("[%s] get EFUSE_AR_VER fail\n", __func__);
		goto error;
	}
	latest_efuse_ar_ver = mtk_ar_table_num_entry - 1;

	if (efuse_ar_ver > latest_efuse_ar_ver) {
		ERROR("[%s] check EFUSE_AR_VER fail, "
		      "can't greater than LATEST_EFUSE_AR_VER\n", __func__);
		goto error;
	}

	return mtk_ar_table[efuse_ar_ver].ntfw_ar_ver;

error:
	return -1;
}

int mtk_antirollback_get_fit_ar_ver(uint32_t *fit_ar_ver)
{
	int ret;
	uint32_t efuse_ar_ver;
	uint32_t latest_efuse_ar_ver;

	ret = mtk_antirollback_check_mtk_ar_table_parms();
	if (ret)
		goto error;

	ret = mtk_antirollback_get_efuse_ar_ver(&efuse_ar_ver);
	if (ret) {
		ERROR("[%s] get EFUSE_AR_VER fail\n", __func__);
		goto error;
	}
	latest_efuse_ar_ver = mtk_ar_table_num_entry - 1;

	if (efuse_ar_ver > latest_efuse_ar_ver) {
		ERROR("[%s] check EFUSE_AR_VER fail, "
		      "can't greater than LATEST_EFUSE_AR_VER\n", __func__);
		goto error;
	}

	*fit_ar_ver = mtk_ar_table[efuse_ar_ver].fit_ar_ver;

	return 0;

error:
	return -1;
}

int mtk_antirollback_update_efuse_ar_ver(void)
{
	int ret;
	uint32_t efuse_ar_ver;
	uint32_t latest_efuse_ar_ver;

	ret = mtk_antirollback_check_mtk_ar_table_parms();
	if (ret)
		goto error;

	ret = mtk_antirollback_get_efuse_ar_ver(&efuse_ar_ver);
	if (ret) {
		ERROR("[%s] get EFUSE_AR_VER fail\n", __func__);
		goto error;
	}
	latest_efuse_ar_ver = mtk_ar_table_num_entry - 1;

	if (efuse_ar_ver > latest_efuse_ar_ver) {
		ERROR("[%s] check EFUSE_AR_VER fail, "
		      "can't greater than LATEST_EFUSE_AR_VER\n", __func__);
		goto error;
	} else if (efuse_ar_ver < latest_efuse_ar_ver) {
		ret = mtk_antirollback_set_efuse_ar_ver(latest_efuse_ar_ver);
		if (ret) {
			ERROR("[%s] set EFUSE_AR_VER fail\n", __func__);
			goto error;
		}
	}

	return 0;

error:
	return -1;
}
