#include <stdio.h>
#include <string.h>
#include <mtk_efuse.h>
#include <efuse_cmd.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <lib/xlat_tables/xlat_tables_v2.h>

#ifndef DRAM_BASE
#define DRAM_BASE		0x40000000ULL
#endif
#define DRAM_MAX_SIZE		0x200000000ULL

uint32_t mtk_efuse_get_len(uint32_t efuse_field,
			   uint32_t *efuse_len_ptr)
{
	return efuse_get_len(efuse_field, efuse_len_ptr);
}

uint32_t mtk_efuse_read(uint32_t efuse_field,
			uint8_t *efuse_buffer,
			uint32_t efuse_buffer_len)
{
	int32_t ret;
	uint32_t efuse_len = 0;

	if (!efuse_buffer)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	ret = mtk_efuse_get_len(efuse_field, &efuse_len);
	if (ret != MTK_EFUSE_SUCCESS)
		return ret;

	if (efuse_len > efuse_buffer_len)
		return MTK_EFUSE_ERROR_EFUSE_LEN_EXCEED_BUFFER_LEN;

	memset(efuse_buffer, 0x0, efuse_buffer_len);
	ret = efuse_read(efuse_field, efuse_buffer, efuse_len);
	if (ret) {
		ERROR("%s : read efuse field (%d) fail (%d)\n",
		      __func__, efuse_field, ret);
		return ((ret < 0) ? -ret : ret) + MTK_EFUSE_ERROR_CODE_OFFSET;
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_write(uint32_t efuse_field,
			 uint8_t *efuse_buffer,
			 uint32_t efuse_buffer_len)
{
	int32_t ret;
	uint32_t efuse_len = 0;

	if (!efuse_buffer)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	ret = mtk_efuse_get_len(efuse_field, &efuse_len);
	if (ret != MTK_EFUSE_SUCCESS)
		return ret;

	if (efuse_len > efuse_buffer_len)
		return MTK_EFUSE_ERROR_EFUSE_LEN_EXCEED_BUFFER_LEN;

	ret = efuse_write(efuse_field, efuse_buffer, efuse_len);
	memset(efuse_buffer, 0x0, efuse_buffer_len);
	if (ret) {
		ERROR("%s : write efuse field (%d) fail (%d)\n",
		      __func__, efuse_field, ret);
		return ((ret < 0) ? -ret : ret) + MTK_EFUSE_ERROR_CODE_OFFSET;
	}

	return MTK_EFUSE_SUCCESS;
}

uint32_t mtk_efuse_disable(uint32_t efuse_field)
{
	return efuse_disable(efuse_field);
}

#if defined(IMAGE_BL31) || defined(IMAGE_BL32)
uint32_t sip_efuse_read(uint32_t efuse_field, uint32_t ofs, uint32_t *val)
{
	int32_t ret;
	uint32_t efuse_len = 0;
	uint8_t buf[MTK_EFUSE_MAX_DATA_LEN] = { 0 };

	ret = mtk_efuse_get_len(efuse_field, &efuse_len);
	if (ret != MTK_EFUSE_SUCCESS)
		return ret;

	if (ofs >= efuse_len)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	ret = efuse_read(efuse_field, buf, efuse_len);
	if (ret) {
		ERROR("%s : read efuse field (%d) fail (%d)\n",
		      __func__, efuse_field, ret);
		return ((ret < 0) ? -ret : ret) + MTK_EFUSE_ERROR_CODE_OFFSET;
	}

	if (ofs + sizeof(*val) <= efuse_len)
		memcpy(val, buf + ofs, sizeof(*val));
	else
		memcpy(val, buf + ofs, efuse_len - ofs);

	return MTK_EFUSE_SUCCESS;
}

uint32_t sip_efuse_write(uint32_t efuse_field, uintptr_t addr, uint32_t size)
{
	int32_t ret;
	uint32_t efuse_len = 0;
	uint32_t data_size_align;
	uintptr_t data = 0;
	uint8_t buf[MTK_EFUSE_MAX_DATA_LEN] = { 0 };

	ret = mtk_efuse_get_len(efuse_field, &efuse_len);
	if (ret != MTK_EFUSE_SUCCESS)
		return ret;

	if (addr < DRAM_BASE || addr >= DRAM_BASE + DRAM_MAX_SIZE ||
	    (addr & PAGE_SIZE_MASK))
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	if (size < efuse_len || size > PAGE_SIZE ||
	    addr + size >= DRAM_BASE + DRAM_MAX_SIZE)
		return MTK_EFUSE_ERROR_INVALIDE_PARAMTER;

	data_size_align = page_align(size, UP);
	ret = mmap_add_dynamic_region_alloc_va(addr, &data, data_size_align,
					       MT_MEMORY | MT_RO | MT_NS);
	if (ret)
		return ret;

	memcpy(buf, (uint8_t *)data, efuse_len);

	ret = efuse_write(efuse_field, buf, efuse_len);
	if (ret) {
		ERROR("%s : write efuse field (%d) fail (%d)\n",
		      __func__, efuse_field, ret);
	}

	memset(buf, 0x0, sizeof(buf));
	mmap_remove_dynamic_region(data, data_size_align);

	return ret;
}
#endif
