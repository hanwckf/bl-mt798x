/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt MTD-based image upgrading & booting helper
 */

#ifndef _MTD_HELPER_H_
#define _MTD_HELPER_H_

#include <stdbool.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>

struct ubi_volume;

int mtd_erase_skip_bad(struct mtd_info *mtd, u64 offset, u64 size,
		       u64 maxsize, u64 *erasedsize, u64 *eraseend,
		       const char *name, bool fullerase);
int mtd_write_skip_bad(struct mtd_info *mtd, u64 offset, size_t size,
		       u64 maxsize, size_t *writtensize, const void *data,
		       bool verify);
int mtd_read_skip_bad(struct mtd_info *mtd, u64 offset, size_t size,
		      u64 maxsize, size_t *readsize, void *data);

int mtd_update_generic(struct mtd_info *mtd, const void *data, size_t size,
		       bool verify);
int boot_from_mtd(struct mtd_info *mtd, u64 offset, bool do_boot);

void gen_mtd_probe_devices(void);

int ubi_mount_default(void);
int update_ubi_volume_raw(struct ubi_volume *vol, const char *volume,
			  int vol_id, const void *data, size_t size,
			  uint64_t reserved_size, bool dynamic);

void ubi_import_bsp_conf(void);
int ubi_update_bsp_conf(const void *bspconf, uint32_t index);

int mtd_upgrade_image(const void *data, size_t size);
int mtd_boot_image(bool do_boot);

void mtd_boot_set_defaults(void *fdt);

#endif /* _MTD_HELPER_H_ */
