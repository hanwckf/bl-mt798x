/*
 * Copyright (c) 2020, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/debug.h>
#include <common/runtime_svc.h>
#include <lib/utils_def.h>
#include <lib/mmio.h>
#include <mtk_sip_svc.h>
#include <plat_sip_calls.h>
#include <string.h>
#include <spmc.h>
#include <mtk_efuse.h>

uint64_t mt_sip_set_authorized_sreg(uint32_t sreg, uint32_t val)
{
	return MTK_SIP_E_SUCCESS;
}

static uint32_t mt_sip_pwr_on_mtcmos(uint32_t val)
{
	uint32_t ret;

	ret = mtk_spm_non_cpu_ctrl(1, val);
	if (ret)
		return MTK_SIP_E_INVALID_PARAM;
	else
		return MTK_SIP_E_SUCCESS;
}

static uint32_t mt_sip_pwr_off_mtcmos(uint32_t val)
{
	uint32_t ret;

	ret = mtk_spm_non_cpu_ctrl(0, val);
	if (ret)
		return MTK_SIP_E_INVALID_PARAM;
	else
		return MTK_SIP_E_SUCCESS;
}

static uint32_t mt_sip_pwr_mtcmos_support(void)
{
	return MTK_SIP_E_SUCCESS;
}

uintptr_t mediatek_plat_sip_handler(uint32_t smc_fid,
				    u_register_t x1,
				    u_register_t x2,
				    u_register_t x3,
				    u_register_t x4,
				    void *cookie,
				    void *handle,
				    u_register_t flags)
{
	uint32_t ret;
	uint32_t efuse_len = 0;
	uint32_t efuse_data[2] = { x3, x4 };
	static uint32_t efuse_buffer[MTK_EFUSE_PUBK_HASH_INDEX_MAX];

	switch (smc_fid) {
	case MTK_SIP_PWR_ON_MTCMOS:
		ret = mt_sip_pwr_on_mtcmos((uint32_t)x1);
		SMC_RET1(handle, ret);

	case MTK_SIP_PWR_OFF_MTCMOS:
		ret = mt_sip_pwr_off_mtcmos((uint32_t)x1);
		SMC_RET1(handle, ret);

	case MTK_SIP_PWR_MTCMOS_SUPPORT:
		ret = mt_sip_pwr_mtcmos_support();
		SMC_RET1(handle, ret);

	case MTK_SIP_EFUSE_GET_LEN:
		ret = mtk_efuse_get_len((uint32_t)x1, &efuse_len);
		SMC_RET4(handle, ret, efuse_len, 0x0, 0x0);

	case MTK_SIP_EFUSE_SEND_DATA:
		ret = mtk_efuse_send_data((uint8_t *)efuse_buffer,
					  (uint8_t *)efuse_data,
					  (uint32_t)x1,
					  (uint32_t)x2);
		SMC_RET4(handle, ret, x2, 0x0, 0x0);

	case MTK_SIP_EFUSE_GET_DATA:
		ret = mtk_efuse_get_data((uint8_t *)efuse_data,
					 (uint8_t *)efuse_buffer,
					 (uint32_t)x1,
					 (uint32_t)x2);
		SMC_RET4(handle, ret, x2, efuse_data[0], efuse_data[1]);

	case MTK_SIP_EFUSE_READ:
		ret = mtk_efuse_read((uint32_t)x1,
				     (uint8_t *)efuse_buffer,
				     sizeof(efuse_buffer));
		SMC_RET4(handle, ret, 0x0, 0x0, 0x0);

	case MTK_SIP_EFUSE_WRITE:
		ret = mtk_efuse_write((uint32_t)x1,
				      (uint8_t *)efuse_buffer,
				      sizeof(efuse_buffer));
		SMC_RET4(handle, ret, 0x0, 0x0, 0x0);

	default:
		ERROR("%s: unhandled SMC (0x%x)\n", __func__, smc_fid);
		break;
	}

	SMC_RET1(handle, SMC_UNK);
}
