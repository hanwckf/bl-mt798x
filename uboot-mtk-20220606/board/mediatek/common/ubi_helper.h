/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * OpenWrt UBI image upgrading & booting helper
 */

#ifndef _UBI_HELPER_H_
#define _UBI_HELPER_H_

void ubi_probe_mtd_devices(void);
int ubi_upgrade_image(const void *data, size_t size);
int ubi_boot_image(void);

#endif /* _UBI_HELPER_H_ */
