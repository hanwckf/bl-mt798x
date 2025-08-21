// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <env.h>
#include <errno.h>

#include "bootmenu_common.h"
#include "colored_print.h"
#include "bl2_helper.h"
#include "fip_helper.h"
#include "verify_helper.h"

int generic_invalidate_env(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size)
{
	int ret = 0;

#if !defined(CONFIG_MTK_SECURE_BOOT) && !defined(CONFIG_ENV_IS_NOWHERE) && \
    !defined(CONFIG_MTK_DUAL_BOOT)
	ret = env_set("env_invalid", "1");
	if (ret)
		return ret;

	ret = env_save();
#endif

	return ret;
}

int generic_validate_bl2(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_BL2_VERIFY)) {
		ret = bl2_check_image_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** BL2 verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

int generic_validate_fip(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_FIP_VERIFY)) {
		ret = fip_check_uboot_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** FIP verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

int generic_validate_bl31(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_FIP_VERIFY)) {
		ret = check_bl31_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** BL31 verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

int generic_validate_bl33(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_FIP_VERIFY)) {
		ret = check_uboot_data(data, size);
		if (ret) {
			cprintln(ERROR, "*** BL33 verification failed ***");
			ret = -EBADMSG;
		}
	}

	return ret;
}

int generic_validate_simg(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	int ret;

	ret = generic_validate_bl2(priv, dpe, data, size);
	if (ret) {
		cprintln(ERROR, "*** Single image must include bl2 image ***");
		cprintln(ERROR, "*** Single image verification failed ***");
	}

	return ret;
}
