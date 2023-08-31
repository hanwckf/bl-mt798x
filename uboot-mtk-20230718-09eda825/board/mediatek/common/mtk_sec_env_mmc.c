// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Mediatek Incorporation. All Rights Reserved.
 */
#include <malloc.h>
#include <stdint.h>
#include <u-boot/crc.h>
#include "mmc_helper.h"
#include "mtk_sec_env.h"

static int sec_env_mmc_read(uint32_t dev, uint32_t hwpart, void *buf)
{
	int ret = 0;
	struct mmc *mmc;
	struct disk_partition dpart;

	if (!buf)
		return -EINVAL;

	mmc = _mmc_get_dev(dev, hwpart, false);
	if (!mmc)
		return -ENODEV;

	ret = _mmc_find_part(mmc, PART_U_BOOT_ENV_NAME, &dpart);
	if (ret)
		return ret;

	return _mmc_read(mmc, dpart.start * dpart.blksz + CONFIG_ENV_SIZE / 2,
			 buf, CONFIG_ENV_SIZE / 2);
}

static int sec_env_mmc_load(uint32_t dev, uint32_t hwpart)
{
	int ret = 0;
	char *buf = NULL;

	buf = calloc(CONFIG_ENV_SIZE / 2, sizeof(uint8_t));
	if (!buf)
		return -ENOMEM;

	ret = sec_env_mmc_read(dev, hwpart, buf);
	if (ret) {
		printf("Failed to read sec env from MMC\n");
		goto sec_env_mmc_load_err;
	}

	ret = sec_env_import(buf, CONFIG_ENV_SIZE / 2 - ENV_HEADER_SIZE);
	if (ret)
		printf("Failed to import sec env\n");

sec_env_mmc_load_err:
	if (buf)
		free(buf);

	return ret;
}

int board_sec_env_load(void)
{
	return sec_env_mmc_load(0, 0);
}
