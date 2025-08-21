// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 */

#include <assert.h>
#include <tf_unxz.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <common/image_decompress.h>
#include <common/tbbr/tbbr_img_def.h>
#include <plat/common/common_def.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <tools_share/firmware_image_package.h>
#include <hsuart.h>
#include <platform_def.h>
#include <plat_private.h>
#include <drivers/io/io_encrypted.h>
#include "bl2_plat_setup.h"
#ifdef DUAL_FIP
#include "bsp_conf.h"
#endif

#ifdef MTK_IMG_ENC
#include <img_dec.h>
#endif

#ifdef MTK_PLAT_KEY
#include <mtk_plat_key.h>
#endif

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static size_t dram_size;
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_image_id = FIP_IMAGE_ID;
static uintptr_t fip_dev_handle;
static uintptr_t boot_dev_handle;
#ifdef MTK_MMC_BOOT
static uintptr_t gpt_dev_handle;
#endif
#if MTK_FIP_ENC && !defined(DECRYPTION_SUPPORT_none)
static const io_dev_connector_t *enc_dev_con;
static uintptr_t enc_dev_handle;
#endif

#ifndef MTK_PLAT_NO_DEFAULT_BL2_NEXT_IMAGES
static bl_mem_params_node_t bl2_mem_params_descs[] = {
	/* Fill BL31 related information */
	{
		.image_id = BL31_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      SECURE | EXECUTABLE | EP_FIRST_EXE),
		.ep_info.pc = BL31_BASE,
		.ep_info.spsr = SPSR_64(MODE_EL3, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, IMAGE_ATTRIB_PLAT_SETUP),
		.image_info.image_base = BL31_BASE,
		.image_info.image_max_size = BL31_LIMIT - BL31_BASE,

#ifdef NEED_BL32
		.next_handoff_image_id = BL32_IMAGE_ID,
#else
		.next_handoff_image_id = BL33_IMAGE_ID,
#endif
	},
#ifdef NEED_BL32
	/* Fill BL32 related information */
	{
		.image_id = BL32_IMAGE_ID,

		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t, SECURE | EXECUTABLE),
		.ep_info.pc = BL32_BASE,

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_base = BL32_BASE - BL32_HEADER_SIZE,
		.image_info.image_max_size = BL32_LIMIT - BL32_BASE,

		.next_handoff_image_id = BL33_IMAGE_ID,
	},
#endif
	/* Fill BL33 related information */
	{
		.image_id = BL33_IMAGE_ID,
		SET_STATIC_PARAM_HEAD(ep_info, PARAM_EP, VERSION_2,
				      entry_point_info_t,
				      NON_SECURE | EXECUTABLE),
		.ep_info.pc = BL33_BASE,
		.ep_info.spsr = SPSR_64(MODE_EL2, MODE_SP_ELX,
					DISABLE_ALL_EXCEPTIONS),

		SET_STATIC_PARAM_HEAD(image_info, PARAM_EP, VERSION_2,
				      image_info_t, 0),
		.image_info.image_base = BL33_BASE,
		.image_info.image_max_size = 0x200000 /* 2MB */,

		.next_handoff_image_id = INVALID_IMAGE_ID,
	}
};

REGISTER_BL_IMAGE_DESCS(bl2_mem_params_descs)
#endif /* MTK_PLAT_NO_DEFAULT_BL2_NEXT_IMAGES */

static int check_boot_dev(const uintptr_t spec)
{
	return io_dev_init(boot_dev_handle, (uintptr_t)NULL);
}

#ifdef MTK_MMC_BOOT
static int check_gpt_dev(const uintptr_t spec)
{
	return io_dev_init(gpt_dev_handle, (uintptr_t)NULL);
}
#endif

#if MTK_FIP_ENC && !defined(DECRYPTION_SUPPORT_none)
static int check_enc_fip(const uintptr_t spec)
{
	int result;

	/* See if a Firmware Image Package is available */
	result = io_dev_open(enc_dev_con, (uintptr_t)NULL, &enc_dev_handle);
	if (result) {
		ERROR("io_dev_open failed for FIP (%d)\n", result);
		return result;
	}

	result = io_dev_init(enc_dev_handle, (uintptr_t)ENC_IMAGE_ID);

	return result;
}
#endif


static int check_fip(const uintptr_t spec)
{
	int ret;

	ret = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	if (ret) {
		ERROR("io_dev_open failed for FIP (%d)\n", ret);
		return ret;
	}

	ret = io_dev_init(fip_dev_handle, fip_image_id);
	if (ret) {
		ERROR("io_dev_init failed for FIP image id %lu (%d)\n",
		      fip_image_id, ret);
		io_dev_close(fip_dev_handle);
	}

	return ret;
}

static const io_uuid_spec_t bl31_uuid_spec = {
	.uuid = UUID_EL3_RUNTIME_FIRMWARE_BL31,
};

static const io_uuid_spec_t bl32_uuid_spec = {
	.uuid = UUID_SECURE_PAYLOAD_BL32,
};

static const io_uuid_spec_t bl33_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FIRMWARE_BL33,
};

#if TRUSTED_BOARD_BOOT
static const io_uuid_spec_t trusted_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_KEY_CERT,
};

static const io_uuid_spec_t scp_fw_key_cert_uuid_spec = {
	.uuid = UUID_SCP_FW_KEY_CERT,
};

static const io_uuid_spec_t soc_fw_key_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_KEY_CERT,
};

static const io_uuid_spec_t tos_fw_key_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_OS_FW_KEY_CERT,
};

static const io_uuid_spec_t nt_fw_key_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_KEY_CERT,
};

static const io_uuid_spec_t scp_fw_cert_uuid_spec = {
	.uuid = UUID_SCP_FW_CONTENT_CERT,
};

static const io_uuid_spec_t soc_fw_cert_uuid_spec = {
	.uuid = UUID_SOC_FW_CONTENT_CERT,
};

static const io_uuid_spec_t tos_fw_cert_uuid_spec = {
	.uuid = UUID_TRUSTED_OS_FW_CONTENT_CERT,
};

static const io_uuid_spec_t nt_fw_cert_uuid_spec = {
	.uuid = UUID_NON_TRUSTED_FW_CONTENT_CERT,
};
#endif /* TRUSTED_BOARD_BOOT */

static struct plat_io_policy policies[] = {
	[FIP_IMAGE_ID] = {
		&boot_dev_handle,
		(uintptr_t)NULL,
		check_boot_dev
	},
#if ENCRYPT_BL31 && !defined(DECRYPTION_SUPPORT_none)
	[BL31_IMAGE_ID] = {
		&enc_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		check_enc_fip
	},
#else
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		check_fip
	},
#endif
#if ENCRYPT_BL32 && !defined(DECRYPTION_SUPPORT_none)
	[BL32_IMAGE_ID] = {
		&enc_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		check_enc_fip
	},
#else
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		check_fip
	},
#endif
	[BL33_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl33_uuid_spec,
		check_fip
	},
#ifdef MTK_MMC_BOOT
	[GPT_IMAGE_ID] = {
		&gpt_dev_handle,
		(uintptr_t)NULL,
		check_gpt_dev
	},
	[BKUP_GPT_IMAGE_ID] = {
		&gpt_dev_handle,
		(uintptr_t)NULL,
		check_gpt_dev
	},
#endif
#if TRUSTED_BOARD_BOOT
	[TRUSTED_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&trusted_key_cert_uuid_spec,
		check_fip
	},
	[SCP_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_fw_key_cert_uuid_spec,
		check_fip
	},
	[SOC_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&soc_fw_key_cert_uuid_spec,
		check_fip
	},
	[TRUSTED_OS_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&tos_fw_key_cert_uuid_spec,
		check_fip
	},
	[NON_TRUSTED_FW_KEY_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&nt_fw_key_cert_uuid_spec,
		check_fip
	},
	[SCP_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&scp_fw_cert_uuid_spec,
		check_fip
	},
	[SOC_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&soc_fw_cert_uuid_spec,
		check_fip
	},
	[TRUSTED_OS_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&tos_fw_cert_uuid_spec,
		check_fip
	},
	[NON_TRUSTED_FW_CONTENT_CERT_ID] = {
		&fip_dev_handle,
		(uintptr_t)&nt_fw_cert_uuid_spec,
		check_fip
	},
#if MTK_FIP_ENC && !defined(DECRYPTION_SUPPORT_none)
	[ENC_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)NULL,
		check_fip
	},
#endif
#endif /* TRUSTED_BOARD_BOOT */
};

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;
	int ret;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	ret = policy->check(policy->image_spec);
	if (ret) {
		ERROR("Image id %u open failed with %d\n", image_id, ret);
		return ret;
	}

	*image_spec = policy->image_spec;
	*dev_handle = *policy->dev_handle;

	return 0;
}

static struct image_info *get_image_info(unsigned int image_id)
{
	struct bl_mem_params_node *desc;

	desc = get_bl_mem_params_node(image_id);
	if (!desc)
		return NULL;

	return &desc->image_info;
}

int bl2_plat_handle_pre_image_load(unsigned int image_id)
{
	struct image_info *image_info;

	image_info = get_image_info(image_id);
	if (!image_info)
		return -ENODEV;

	if (!(image_info->h.attr & IMAGE_ATTRIB_SKIP_LOADING))
		image_decompress_prepare(image_info);

	return 0;
}

int bl2_plat_handle_post_image_load(unsigned int image_id)
{
	struct image_info *image_info = get_image_info(image_id);
	int ret;

	if (!image_info)
		return -ENODEV;

	if (!(image_info->h.attr & IMAGE_ATTRIB_SKIP_LOADING)) {
		ret = image_decompress(image_info);
		if (ret)
			return ret;
	}

	return 0;
}

void bl2_plat_handle_post_image_load_err(unsigned int image_id)
{
	struct image_info *image_info = get_image_info(image_id);

	if (!image_info)
		return;

	if (!(image_info->h.attr & IMAGE_ATTRIB_SKIP_LOADING))
		image_decompress_restore(image_info);
}

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	static struct bl_load_info *info = NULL;

	if (!info)
		info = get_bl_load_info_from_mem_params_desc();

	return info;
}

struct bl_params *plat_get_next_bl_params(void)
{
	struct bl_params *params = get_next_bl_params_from_mem_params_desc();

	if (params) {
		params->head->ep_info->args.arg1 = dram_size;
#ifdef MTK_IMG_ENC
		img_dec_set_next_bl_params(&params->head->ep_info->args.arg2,
					   &params->head->ep_info->args.arg3);
#endif
	}

	return params;
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();

#ifdef DUAL_FIP
	finalize_bsp_conf(BL33_BASE);
#endif
}

void bl2_el3_early_platform_setup(u_register_t arg0, u_register_t arg1,
				  u_register_t arg2, u_register_t arg3)
{
	static console_t console;

	console_hsuart_register(UART_BASE, UART_CLOCK, UART_BAUDRATE,
			        true, &console);
}

static void bl2_run_initcalls(void)
{
	const struct initcall *ic = bl2_initcalls;

	while (ic->callfn) {
#if (LOG_LEVEL >= LOG_LEVEL_VERBOSE) && FPGA
		if (ic->name)
			VERBOSE("initcall: [%p] %s\n", ic->callfn, ic->name);
#endif
		ic->callfn();
		ic++;
	}
}

static int bl2_fip_boot_setup(void)
{
	int ret;

#ifdef MTK_MMC_BOOT
	ret = mtk_mmc_gpt_image_setup(&gpt_dev_handle,
				      &policies[GPT_IMAGE_ID].image_spec,
				      &policies[BKUP_GPT_IMAGE_ID].image_spec);
	if (ret)
		return ret;
#endif

	ret = mtk_fip_image_setup(&boot_dev_handle,
				  &policies[FIP_IMAGE_ID].image_spec);
	if (ret)
		return ret;

	ret = register_io_dev_fip(&fip_dev_con);
	if (ret) {
		ERROR("register_io_dev_fip failed, ret: %d\n", ret);
		return ret;
	}

#if MTK_FIP_ENC && !defined(DECRYPTION_SUPPORT_none)
	ret = register_io_dev_enc(&enc_dev_con);
	if (ret)
		ERROR("register_io_dev_enc failed, ret: %d\n", ret);
#endif

	return ret;
}

void mtk_fip_location(size_t *fip_off, size_t *fip_size)
{
#if defined(OVERRIDE_FIP_BASE) && defined(OVERRIDE_FIP_SIZE)
	*fip_off = OVERRIDE_FIP_BASE;
	*fip_size = OVERRIDE_FIP_SIZE;
#else
	mtk_plat_fip_location(fip_off, fip_size);
#endif
}

#if ENABLE_PIE
static void adjust_bl31_load_address(void)
{
	uintptr_t image_base, ram_top = DRAM_BASE + dram_size;
	bl_mem_params_node_t *param;
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(bl2_mem_params_descs); i++) {
		param = &bl2_mem_params_descs[i];

		if (param->image_id == BL31_IMAGE_ID) {
			image_base = ram_top - TZRAM_SIZE;
#ifdef BL31_RSVD_SIZE
			image_base -= BL31_RSVD_SIZE;
#endif
			param->image_info.image_base = image_base;
			param->image_info.image_base &= ~(SZ_64K - 1);
			param->image_info.image_base += BL31_LOAD_OFFSET;
			param->ep_info.pc = param->image_info.image_base;

			VERBOSE("BL31 base address set to 0x%08zx\n",
				param->ep_info.pc);

			break;
		}
	}
}
#endif

void bl2_plat_preload_setup(void)
{
	int ret;

	bl2_run_initcalls();

	ret = bl2_fip_boot_setup();
	if (ret) {
		ERROR("FIP boot source initialization failed with %d\n", ret);
		panic();
	}

	image_decompress_init(FIP_DECOMP_BUF_OFFSET, FIP_DECOMP_BUF_SIZE, unxz);

#if ENABLE_PIE
	adjust_bl31_load_address();
#endif
}

void bl2_platform_setup(void)
{
}

void mtk_bl2_set_dram_size(size_t size)
{
	dram_size = size;
}

void bl2_el3_plat_prepare_exit(void)
{
#ifdef MTK_PLAT_KEY
	disable_plat_key();
#endif
}
