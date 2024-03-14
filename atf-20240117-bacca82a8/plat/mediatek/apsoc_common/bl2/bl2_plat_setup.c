/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <tf_unxz.h>
#include <arch_helpers.h>
#include <common/debug.h>
#include <common/desc_image_load.h>
#include <common/image_decompress.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_fip.h>
#include <tools_share/firmware_image_package.h>
#include <hsuart.h>
#include <platform_def.h>
#include <plat_private.h>
#include "bl2_plat_setup.h"

struct plat_io_policy {
	uintptr_t *dev_handle;
	uintptr_t image_spec;
	int (*check)(const uintptr_t spec);
};

static size_t dram_size;
static const io_dev_connector_t *fip_dev_con;
static uintptr_t fip_dev_handle;
static uintptr_t boot_dev_handle;
#ifdef MTK_MMC_BOOT
static uintptr_t gpt_dev_handle;
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

static int check_fip(const uintptr_t spec)
{
	int ret;

	ret = io_dev_open(fip_dev_con, (uintptr_t)NULL, &fip_dev_handle);
	if (ret)
		return ret;

	return io_dev_init(fip_dev_handle, (uintptr_t)FIP_IMAGE_ID);
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
	[BL31_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl31_uuid_spec,
		check_fip
	},
	[BL32_IMAGE_ID] = {
		&fip_dev_handle,
		(uintptr_t)&bl32_uuid_spec,
		check_fip
	},
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
#endif /* TRUSTED_BOARD_BOOT */
};

int plat_get_image_source(unsigned int image_id, uintptr_t *dev_handle,
			  uintptr_t *image_spec)
{
	const struct plat_io_policy *policy;

	assert(image_id < ARRAY_SIZE(policies));

	policy = &policies[image_id];
	policy->check(policy->image_spec);

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

void bl2_plat_preload_setup(void)
{
	image_decompress_init(FIP_DECOMP_BUF_OFFSET, FIP_DECOMP_BUF_SIZE, unxz);
}

int bl2_plat_handle_pre_image_load(unsigned int image_id)
{
	struct image_info *image_info;

	image_info = get_image_info(image_id);
	if (!image_info)
		return -ENODEV;

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

struct bl_load_info *plat_get_bl_image_load_info(void)
{
	return get_bl_load_info_from_mem_params_desc();
}

struct bl_params *plat_get_next_bl_params(void)
{
	struct bl_params *params = get_next_bl_params_from_mem_params_desc();

	if (params)
		params->head->ep_info->args.arg1 = dram_size;

	return params;
}

void plat_flush_next_bl_params(void)
{
	flush_bl_params_desc();
}

void platform_mem_init(void)
{
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

void bl2_platform_setup(void)
{
	int ret;

	bl2_run_initcalls();

	ret = bl2_fip_boot_setup();
	if (ret) {
		ERROR("FIP boot source initialization failed with %d\n", ret);
		panic();
	}
}

void mtk_bl2_set_dram_size(size_t size)
{
	dram_size = size;
}
