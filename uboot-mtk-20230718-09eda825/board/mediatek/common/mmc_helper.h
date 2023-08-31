/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt SD/eMMC image upgrading & booting helper
 */

#ifndef _MMC_HELPER_H_
#define _MMC_HELPER_H_

#include <linux/types.h>
#include <mmc.h>
#include <part.h>

struct mmc *_mmc_get_dev(u32 dev, int hwpart, bool write);

int _mmc_write(struct mmc *mmc, u64 offset, size_t max_size, const void *data,
	       size_t size, bool verify);
int _mmc_read(struct mmc *mmc, u64 offset, void *data, size_t size);
int _mmc_erase(struct mmc *mmc, u64 offset, size_t max_size, u64 size);

int mmc_write_generic(u32 dev, int hwpart, u64 offset, size_t max_size,
		      const void *data, size_t size, bool verify);
int mmc_read_generic(u32 dev, int hwpart, u64 offset, void *data, size_t size);
int mmc_erase_generic(u32 dev, int hwpart, u64 offset, u64 size);
int mmc_erase_env_generic(u32 dev, int hwpart, u64 offset, u64 size);

int _mmc_find_part(struct mmc *mmc, const char *part_name,
		   struct disk_partition *dpart);
int mmc_write_part(u32 dev, int hwpart, const char *part_name, const void *data,
		   size_t size, bool verify);
int mmc_read_part(u32 dev, int hwpart, const char *part_name, void *data,
		  size_t size);
int mmc_read_part_size(u32 dev, int hwpart, const char *part_name, u64 *size);
int mmc_erase_part(u32 dev, int hwpart, const char *part_name, u64 offset,
		   u64 size);
int mmc_erase_env_part(u32 dev, int hwpart, const char *part_name, u64 size);

int mmc_write_gpt(u32 dev, int hwpart, size_t max_size, const void *data,
		  size_t size);

void mmc_setup_boot_options(u32 dev);
int mmc_is_sd(u32 dev);

int boot_from_mmc_partition(u32 dev, u32 hwpart, const char *part_name);

int mmc_boot_image(u32 dev);
int mmc_upgrade_image(u32 dev, const void *data, size_t size);

#endif /* _MMC_HELPER_H_ */
