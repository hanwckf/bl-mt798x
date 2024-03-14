/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch/aarch64/arch.h>
#include <arch/aarch64/arch_helpers.h>
#include <common/runtime_svc.h>
#include "mtk_boot_next.h"
#include "mtk_sip_svc.h"
#include "apsoc_sip_svc_common.h"
#include <mtk_efuse.h>
#ifdef MTK_FSEK
#include <mtk_fsek.h>
#endif
#include "memdump.h"

#if MTK_SIP_KERNEL_BOOT_ENABLE
static uintptr_t apsoc_sip_boot_to_kernel(uint32_t smc_fid, u_register_t x1,
					  u_register_t x2, u_register_t x3,
					  u_register_t x4, void *cookie,
					  void *handle, u_register_t flags)
{
	boot_to_kernel(x1, x2, x3, x4);
	SMC_RET0(handle);
}
#endif

static uintptr_t apsoc_sip_efuse_get_len(uint32_t smc_fid, u_register_t x1,
					 u_register_t x2, u_register_t x3,
					 u_register_t x4, void *cookie,
					 void *handle, u_register_t flags)
{
	uint32_t efuse_len = 0;
	uintptr_t ret;

	ret = mtk_efuse_get_len((uint32_t)x1, &efuse_len);
	SMC_RET4(handle, ret, efuse_len, 0x0, 0x0);
}

static uint32_t efuse_buffer[MTK_EFUSE_PUBK_HASH_INDEX_MAX];

static uintptr_t apsoc_sip_efuse_send_data(uint32_t smc_fid, u_register_t x1,
					   u_register_t x2, u_register_t x3,
					   u_register_t x4, void *cookie,
					   void *handle, u_register_t flags)
{
	uint32_t efuse_data[2] = { (uint32_t)x3, (uint32_t)x4 };
	uintptr_t ret;

	ret = mtk_efuse_send_data((uint8_t *)efuse_buffer,
				  (uint8_t *)efuse_data,
				  (uint32_t)x1,
				  (uint32_t)x2);
	SMC_RET4(handle, ret, x2, 0x0, 0x0);
}

static uintptr_t apsoc_sip_efuse_get_data(uint32_t smc_fid, u_register_t x1,
					  u_register_t x2, u_register_t x3,
					  u_register_t x4, void *cookie,
					  void *handle, u_register_t flags)
{
	uint32_t efuse_data[2] = { (uint32_t)x3, (uint32_t)x4 };
	uintptr_t ret;

	ret = mtk_efuse_get_data((uint8_t *)efuse_data,
				 (uint8_t *)efuse_buffer,
				 (uint32_t)x1,
				 (uint32_t)x2);
	SMC_RET4(handle, ret, x2, efuse_data[0], efuse_data[1]);
}

static uintptr_t apsoc_sip_efuse_read(uint32_t smc_fid, u_register_t x1,
				      u_register_t x2, u_register_t x3,
				      u_register_t x4, void *cookie,
				      void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtk_efuse_read((uint32_t)x1,
			     (uint8_t *)efuse_buffer,
			     sizeof(efuse_buffer));
	SMC_RET4(handle, ret, 0x0, 0x0, 0x0);
}

static uintptr_t apsoc_sip_efuse_write(uint32_t smc_fid, u_register_t x1,
				       u_register_t x2, u_register_t x3,
				       u_register_t x4, void *cookie,
				       void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtk_efuse_write((uint32_t)x1,
			      (uint8_t *)efuse_buffer,
			      sizeof(efuse_buffer));
	SMC_RET4(handle, ret, 0x0, 0x0, 0x0);
}

static uintptr_t apsoc_sip_efuse_disable(uint32_t smc_fid, u_register_t x1,
					 u_register_t x2, u_register_t x3,
					 u_register_t x4, void *cookie,
					 void *handle, u_register_t flags)
{
	uintptr_t ret;

	ret = mtk_efuse_disable((uint32_t)x1);
	SMC_RET4(handle, ret, 0x0, 0x0, 0x0);
}

#ifdef MTK_FSEK
static uintptr_t apsoc_sip_fsek_get_shm_config(uint32_t smc_fid,
					       u_register_t x1,
					       u_register_t x2, u_register_t x3,
					       u_register_t x4, void *cookie,
					       void *handle, u_register_t flags)
{
	u_register_t buffer[4] = {0};
	uintptr_t ret;

	if (GET_EL(read_spsr_el3()) == MODE_EL2) {
		ret = mtk_fsek_get_shm_config((uintptr_t *)&buffer[0],
					      (size_t *)&buffer[1]);
		SMC_RET3(handle, ret, buffer[0], buffer[1]);
	}

	SMC_RET3(handle, MTK_FSEK_ERR_WRONG_SRC, 0x0, 0x0);
}

static uintptr_t apsoc_sip_fsek_decrypt_rfsk(uint32_t smc_fid,
					     u_register_t x1,
					     u_register_t x2, u_register_t x3,
					     u_register_t x4, void *cookie,
					     void *handle, u_register_t flags)
{
	uintptr_t ret;

	if (GET_EL(read_spsr_el3()) == MODE_EL2) {
		ret = mtk_fsek_decrypt_rfsk();
		SMC_RET1(handle, ret);
	}

	SMC_RET1(handle, MTK_FSEK_ERR_WRONG_SRC);
}

static uintptr_t apsoc_sip_fsek_get_key(uint32_t smc_fid,
					u_register_t x1,
					u_register_t x2, u_register_t x3,
					u_register_t x4, void *cookie,
					void *handle, u_register_t flags)
{
	u_register_t buffer[4] = {0};
	uintptr_t ret;

	ret = mtk_fsek_get_key((uint32_t)x1, (void *)buffer, sizeof(buffer));
	if (!ret) {
		SMC_RET4(handle, buffer[0], buffer[1], buffer[2], buffer[3]);
	}

	SMC_RET4(handle, MTK_FSEK_ERR_WRONG_SRC, 0x0, 0x0, 0x0);
}

static uintptr_t apsoc_sip_fsek_encrypt_roek(uint32_t smc_fid,
					     u_register_t x1,
					     u_register_t x2, u_register_t x3,
					     u_register_t x4, void *cookie,
					     void *handle, u_register_t flags)
{
	u_register_t buffer[4] = {0};
	uintptr_t ret;

	ret = mtk_fsek_encrypt_roek(x1, x2, x3, x4,
				    (void *)buffer, sizeof(buffer));
	if (!ret) {
		SMC_RET4(handle, buffer[0], buffer[1], buffer[2], buffer[3]);
	}

	SMC_RET4(handle, ret, 0x0, 0x0, 0x0);
}
#endif /* MTK_FSEK */

#ifdef EMERG_MEM_DUMP
static uintptr_t apsoc_sip_emerg_mem_dump(uint32_t smc_fid, u_register_t x1,
					  u_register_t x2, u_register_t x3,
					  u_register_t x4, void *cookie,
					  void *handle, u_register_t flags)
{
	do_mem_dump((int)x1, handle);
	SMC_RET0(handle);
}
#endif

struct mtk_sip_call_record apsoc_common_sip_calls[] = {
#ifdef MTK_SIP_KERNEL_BOOT_ENABLE
	MTK_SIP_CALL_RECORD(MTK_SIP_KERNEL_BOOT_AARCH32, apsoc_sip_boot_to_kernel),
#endif
	MTK_SIP_CALL_RECORD(MTK_SIP_EFUSE_GET_LEN, apsoc_sip_efuse_get_len),
	MTK_SIP_CALL_RECORD(MTK_SIP_EFUSE_SEND_DATA, apsoc_sip_efuse_send_data),
	MTK_SIP_CALL_RECORD(MTK_SIP_EFUSE_GET_DATA, apsoc_sip_efuse_get_data),
	MTK_SIP_CALL_RECORD(MTK_SIP_EFUSE_READ, apsoc_sip_efuse_read),
	MTK_SIP_CALL_RECORD(MTK_SIP_EFUSE_WRITE, apsoc_sip_efuse_write),
	MTK_SIP_CALL_RECORD(MTK_SIP_EFUSE_DISABLE, apsoc_sip_efuse_disable),
#ifdef MTK_FSEK
	MTK_SIP_CALL_RECORD(MTK_SIP_FSEK_GET_SHM_CONFIG, apsoc_sip_fsek_get_shm_config),
	MTK_SIP_CALL_RECORD(MTK_SIP_FSEK_DECRYPT_RFSK, apsoc_sip_fsek_decrypt_rfsk),
	MTK_SIP_CALL_RECORD(MTK_SIP_FSEK_GET_KEY, apsoc_sip_fsek_get_key),
	MTK_SIP_CALL_RECORD(MTK_SIP_FSEK_ENCRYPT_ROEK, apsoc_sip_fsek_encrypt_roek),
#endif /* MTK_FSEK */
#ifdef EMERG_MEM_DUMP
	MTK_SIP_CALL_RECORD(MTK_SIP_EMERG_MEM_DUMP, apsoc_sip_emerg_mem_dump),
#endif
};

const uint32_t apsoc_common_sip_call_num = ARRAY_SIZE(apsoc_common_sip_calls);
