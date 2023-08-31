/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#ifndef _FIP_HELPER_H_
#define _FIP_HELPER_H_

#include "fip.h"
#include "upgrade_helper.h"

/* 16MB fip max size */
#define MAX_FIP_SIZE		0x1000000

/* 2MB u-boot max size */
#define MAX_UBOOT_SIZE		0x200000

/* fip buffer */
#define FIP_BUFFER_ADDR		(CONFIG_SYS_LOAD_ADDR + MAX_FIP_SIZE)

enum fip_buffer {
	FIP_READ_BUFFER,
	FIP_WRITE_BUFFER,
	__FIP_BUFFER_NUM,
};

int get_fip_buffer(enum fip_buffer buffer_type, void **buffer, size_t *size);

/* utils */
int fip_check_uboot_data(const void *fip_data, ulong fip_size);
int check_bl31_data(const void *bl31_data, ulong bl31_size);
int check_uboot_data(const void *uboot_data, ulong uboot_size);
int fip_update_bl31_data(const void *bl31_data, ulong bl31_size,
			 const void *fip_data, ulong fip_size,
			 ulong *new_fip_size, ulong max_size);
int fip_update_uboot_data(const void *uboot_data, ulong uboot_size,
			  const void *fip_data, ulong fip_size,
			  ulong *new_fip_size, ulong max_size);

#endif /* _FIP_HELPER_ */
