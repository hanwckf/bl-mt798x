// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <asm/global_data.h>
#include <common.h>
#include <fdt_support.h>
#include <image.h>
#include <linux/libfdt.h>
#include "fip_helper.h"
#include "board_info.h"
#include "colored_print.h"

DECLARE_GLOBAL_DATA_PTR;

int get_fip_buffer(enum fip_buffer buffer_type, void **buffer, size_t *size)
{
	ulong addr;

	if (buffer_type >= __FIP_BUFFER_NUM)
		return -EINVAL;

	addr = FIP_BUFFER_ADDR + MAX_FIP_SIZE * buffer_type;

	*buffer = (void *)(uintptr_t)addr;
	*size = MAX_FIP_SIZE;

	return 0;
}

static const void *locate_fdt_in_uboot(const void *uboot, ulong search_size)
{
	struct legacy_img_hdr *header;
	const u8 *buf = uboot;
	ulong fdt_size;
	ulong offset;

	if (search_size < sizeof(struct legacy_img_hdr))
		return NULL;

	/* use backward search to find fdt magic header and check fdt size */
	offset = search_size - sizeof(struct legacy_img_hdr);
	while (offset) {
		header = (struct legacy_img_hdr *)(buf + offset);
		if (image_get_magic(header) == FDT_MAGIC) {
			fdt_size = fdt_totalsize(buf + offset);
			if (fdt_size <= (search_size - offset))
				return (void *)(buf + offset);
		}
		offset--;
	}

	return NULL;
}

int check_bl31_data(const void *bl31_data, ulong bl31_size)
{
	/*
	 * bl31/aarch64/bl31_entrypoint.S
	 * func bl31_entrypoint
	 * 	mov     x20, x0      |      0xAA0003F4
	 */
	const u32 bl31_magic = 0xAA0003F4;
	u32 magic;

	if (bl31_size < 4)
		goto err;

	memcpy(&magic, bl31_data, sizeof(u32));
	if (magic != bl31_magic)
		goto err;

	return 0;

err:
	cprintln(ERROR, "*** BL31 image not found ***");
	return -EBADMSG;
}

int check_uboot_data(const void *uboot_data, ulong uboot_size)
{
	struct compat_list t_compat, c_compat;
	struct fip working_fip;
	const void *fdt_blob;
	unsigned int count;
	int ret;
	int i;

	/* prevent use fip image as u-boot image */
	ret = init_fip(uboot_data, uboot_size, &working_fip);
	if (!ret) {
		cprintln(ERROR, "*** FIP found in U-boot image ***");
		free_fip(&working_fip);
		return -EBADMSG;
	}

	/* search and get fdt from u-boot binary data */
	fdt_blob = locate_fdt_in_uboot(uboot_data, uboot_size);
	if (!fdt_blob) {
		cprintln(ERROR, "*** FDT not found in U-boot image ***");
		goto err;
	}

	/* read compatible string from target u-boot */
	if (fdt_read_compat_list(fdt_blob, 0, "compatible", &t_compat)) {
		cprintln(ERROR, "*** Compatible string not found ***");
		goto err;
	}

	/* read compatible string from current u-boot */
	if (fdt_read_compat_list(gd->fdt_blob, 0, "compatible", &c_compat)) {
		cprintln(ERROR, "*** Compatible string not found ***");
		goto err;
	}

	if (c_compat.count > t_compat.count)
		count = t_compat.count;
	else
		count = c_compat.count;

	/* check the longer compat list is the superset of the shorter one */
	for (i = 0; i < count; i++)
		if (strcmp(c_compat.compats[i], t_compat.compats[i]))
			break;

	if (i == count)
		return 0;

	cprintln(ERROR, "*** FIP is not compatible with current u-boot ***");
	log_err("       current compatible strings: ");
	print_compat_list(&c_compat);
	log_err("       new u-boot compatible strings: ");
	print_compat_list(&t_compat);

err:
	return -EBADMSG;
}

int fip_check_uboot_data(const void *fip_data, ulong fip_size)
{
	struct fip working_fip;
	const void *uboot_data;
	ulong uboot_size;
	struct fip *fip;

	int ret;

	ret = init_fip(fip_data, fip_size, &working_fip);
	if (ret) {
		cprintln(ERROR, "*** FIP initialization failed (%d) ***", ret);
		return -EBADMSG;
	}
	fip = &working_fip;

	ret = fip_get_image(fip, "u-boot", &uboot_data, &uboot_size);
	if (ret) {
		ret = -EBADMSG;
		cprintln(ERROR, "*** U-boot image not found (%d) ***", ret);
		goto err;
	}

	ret = check_uboot_data(uboot_data, uboot_size);

err:
	free_fip(fip);

	return ret;
}

static int fip_update_data(const char *name, const void *new_data,
			   size_t new_size, const void *fip_data,
			   size_t fip_size, size_t *new_fip_size,
			   size_t max_size)
{
	struct fip working_fip;
	struct fip *fip;
	size_t size;
	void *buf;
	int ret;

	ret = init_fip(fip_data, fip_size, &working_fip);
	if (ret) {
		cprintln(ERROR, "*** FIP initialization failed (%d) ***", ret);
		return -EBADMSG;
	}
	fip = &working_fip;

	ret = fip_update(fip, name, new_data, new_size);
	if (ret) {
		ret = -EBADMSG;
		cprintln(ERROR, "*** FIP update failed (%d) ***", ret);
		goto err;
	}

	ret = get_fip_buffer(FIP_WRITE_BUFFER, &buf, &size);
	if (ret) {
		ret = -ENOBUFS;
		goto err;
	}

	if ((fip->size > max_size) || (fip->size > size)) {
		ret = -ENOBUFS;
		cprintln(ERROR, "*** %s image size is too big ***", name);
		goto err;
	}

	ret = fip_repack(fip, buf, size);
	if (ret) {
		ret = -EBADMSG;
		cprintln(ERROR, "*** FIP repack failed (%d) ***", ret);
		goto err;
	}

	*new_fip_size = fip->size;

err:
	free_fip(fip);

	return ret;
}

int fip_update_bl31_data(const void *bl31_data, size_t bl31_size,
			  const void *fip_data, size_t fip_size,
			  size_t *new_fip_size, size_t max_size)
{
	return fip_update_data("bl31", bl31_data, bl31_size, fip_data,
			       fip_size, new_fip_size, max_size);
}

int fip_update_uboot_data(const void *uboot_data, size_t uboot_size,
			  const void *fip_data, size_t fip_size,
			  size_t *new_fip_size, size_t max_size)
{
	return fip_update_data("u-boot", uboot_data, uboot_size, fip_data,
			       fip_size, new_fip_size, max_size);
}
