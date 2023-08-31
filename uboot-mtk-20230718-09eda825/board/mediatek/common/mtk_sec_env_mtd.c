// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Mediatek Incorporation. All Rights Reserved.
 */
#include <linux/mtd/mtd.h>
#include <errno.h>
#include <env_internal.h>
#include <malloc.h>
#include <mtd.h>
#include <stdint.h>
#include <u-boot/crc.h>
#include "mtd_helper.h"
#include "mtk_sec_env.h"

static struct mtd_info *env_mtd_get_dev(void)
{
	struct mtd_info *mtd;

	mtd_probe_devices();

	mtd = get_mtd_device_nm(PART_U_BOOT_ENV_NAME);
	if (IS_ERR_OR_NULL(mtd)) {
		printf("MTD device '%s' not found\n", PART_U_BOOT_ENV_NAME);
		return NULL;
	}

	return mtd;
}

static int sec_env_mtd_read(void *buf)
{
	int ret = 0;
	struct mtd_info *mtd;

	if (!buf)
		return -EINVAL;

	mtd = env_mtd_get_dev();
	if (!mtd)
		return -ENODEV;

	ret = mtd_read_skip_bad(mtd, mtd->size / 2, CONFIG_ENV_SIZE,
				mtd->size, NULL, buf);
	put_mtd_device(mtd);

	return ret;
}

static int sec_env_mtd_load(void)
{
	int ret = 0;
	void *buf = NULL;

	buf = calloc(CONFIG_ENV_SIZE, sizeof(uint8_t));
	if (!buf)
		return -ENOMEM;

	ret = sec_env_mtd_read(buf);
	if (ret) {
		printf("Failed to read sec env from MTD\n");
		goto sec_env_mtd_load_err;
	}

	ret = sec_env_import(buf, ENV_SIZE);
	if (ret)
		printf("Failed to import sec env\n");

sec_env_mtd_load_err:
	if (buf)
		free(buf);

	return ret;
}

int board_sec_env_load(void)
{
	return sec_env_mtd_load();
}
