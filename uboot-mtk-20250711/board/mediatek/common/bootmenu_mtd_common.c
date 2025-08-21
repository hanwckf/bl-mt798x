// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2024 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <mtd.h>
#include <ubi_uboot.h>

#include "bootmenu_common.h"
#include "colored_print.h"
#include "mtd_helper.h"
#include "bl2_helper.h"
#include "fip_helper.h"
#include "verify_helper.h"
#include "bsp_conf.h"

#define BL2_REDUND_BLOCKS		2

struct mtd_info *get_mtd_part(const char *partname)
{
	struct mtd_info *mtd;

	gen_mtd_probe_devices();

	if (partname)
		mtd = get_mtd_device_nm(partname);
	else
		mtd = get_mtd_device(NULL, 0);

	if (IS_ERR(mtd)) {
		cprintln(ERROR, "*** MTD partition '%s' not found! ***",
			 partname);
	}

	return mtd;
}

int read_mtd_part(const char *partname, void *data, size_t *size,
		  size_t max_size)
{
	struct mtd_info *mtd;
	u64 part_size;
	int ret;

	mtd = get_mtd_part(partname);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	part_size = mtd->size;

	if (part_size > (u64)max_size) {
		ret = -ENOBUFS;
		goto err;
	}

	*size = (size_t)part_size;

	ret = mtd_read_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, data);

err:
	put_mtd_device(mtd);

	return ret;
}

int write_mtd_part(const char *partname, const void *data, size_t size,
		   bool verify)
{
	struct mtd_info *mtd;
	int ret;

	mtd = get_mtd_part(partname);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_update_generic(mtd, data, size, verify);

	put_mtd_device(mtd);

	return ret;
}

/******************************************************************************/

int generic_mtd_boot_image(bool do_boot)
{
	return mtd_boot_image(do_boot);
}

int generic_mtd_write_bl2(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return write_mtd_part(PART_BL2_NAME, data, size, true);
}

int generic_mtd_write_bl2_redund(void *priv, const struct data_part_entry *dpe,
				 const void *data, size_t size)
{
	u32 redund_size, max_region_size, bl2_ok_count = 0;
	u64 addr, end, max_size, erased_size;
	size_t written_size;
	struct mtd_info *mtd;
	int ret;

	mtd = get_mtd_part(PART_BL2_NAME);
	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	redund_size = BL2_REDUND_BLOCKS * mtd->erasesize;
	max_region_size = 2 * redund_size;
	addr = redund_size;

	if (mtd->size < 2 * ALIGN(size, redund_size))
		goto legacy_update;

	while (addr < mtd->size) {
		max_size = mtd->size - addr;
		if (max_size < size)
			break;

		if (max_size > max_region_size && size <= redund_size)
			max_size = max_region_size;

		written_size = 0;
		erased_size = 0;

		ret = mtd_block_isbad(mtd, addr);
		if (ret > 0) {
			addr += redund_size;
			continue;
		}

		ret = mtd_erase_skip_bad(mtd, addr, size, max_size,
					 &erased_size, &end, NULL, false);
		if (ret) {
			addr += redund_size;
			continue;
		}

		ret = mtd_write_skip_bad(mtd, addr, size, max_size,
					 &written_size, data, true);
		if (ret) {
			if (written_size) {
				mtd_erase_skip_bad(mtd, addr, mtd->erasesize,
						   mtd->erasesize, NULL, NULL,
						   NULL, false);
			}

			addr += redund_size;
			continue;
		}

		addr = ALIGN(end, redund_size);
		bl2_ok_count++;
	}

	if (!bl2_ok_count) {
	legacy_update:
		ret = mtd_update_generic(mtd, data, size, true);
		put_mtd_device(mtd);
		return ret;
	}

	mtd_erase_skip_bad(mtd, 0, redund_size, redund_size, NULL, NULL, NULL,
			   false);

	put_mtd_device(mtd);

	return 0;
}

int generic_mtd_write_fip(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return write_mtd_part(PART_FIP_NAME, data, size, true);
}

#ifdef CONFIG_MTD_UBI
static int ubi_write_fip(struct ubi_volume *vol, const char *volname,
			 const void *data, size_t size)
{
	size_t rsvd_size = 0;
	int ret;

	if (!vol) {
		ret = ubi_mount_default();
		if (ret)
			return ret;

		vol = ubi_find_volume((char *)volname);
	}

	if (vol)
		rsvd_size = vol->reserved_pebs * vol->usable_leb_size;

#ifdef CONFIG_MTK_UBI_FIP_MIN_SIZE
	if (CONFIG_MTK_UBI_FIP_MIN_SIZE > rsvd_size) {
		ret = update_ubi_volume_raw(vol, volname, -1, data, size,
					    CONFIG_MTK_UBI_FIP_MIN_SIZE, false);
		if (!ret)
			return 0;
	}
#endif

	return update_ubi_volume_raw(vol, volname, -1, data, size,
				     rsvd_size, false);
}

int generic_ubi_write_fip(void *priv, const struct data_part_entry *dpe,
			  const void *data, size_t size)
{
	return ubi_write_fip(NULL, PART_FIP_NAME, data, size);
}

#ifdef CONFIG_MTK_DUAL_FIP
int generic_ubi_write_dual_fip(void *priv, const struct data_part_entry *dpe,
			       const void *data, size_t size)
{
	const char *volname = PART_FIP_NAME;
	uint32_t next_slot = 0;
	int ret;

	if (curr_bspconf.current_fip_slot == 0) {
		volname = PART_FIP2_NAME;
		next_slot = 1;
	}

	ret = ubi_write_fip(NULL, volname, data, size);
	if (ret)
		return ret;

	curr_bspconf.current_fip_slot = next_slot;
	curr_bspconf.fip[next_slot].invalid = 0;
	curr_bspconf.fip[next_slot].size = size;
	curr_bspconf.fip[next_slot].crc32 = crc32(0, data, size);
	curr_bspconf.fip[next_slot].upd_cnt++;

	return save_bsp_conf();
}
#endif

int generic_ubi_update_bl31(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	size_t fip_part_size, new_fip_size, buf_size;
	struct ubi_volume *vol;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = ubi_mount_default();
	if (ret)
		return ret;

	vol = ubi_find_volume(PART_FIP_NAME);
	if (!vol) {
		cprintln(ERROR, "*** FIP volume not found ***");
		return -ENODEV;
	}

	fip_part_size = (size_t)vol->used_bytes;
	if (fip_part_size > buf_size) {
		cprintln(ERROR, "*** FIP data is too big ***");
		return -E2BIG;
	}

	ret = ubi_volume_read(PART_FIP_NAME, buf, 0, fip_part_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	ret = fip_update_bl31_data(data, size, buf, fip_part_size,
				   &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return ubi_write_fip(vol, PART_FIP_NAME, buf, new_fip_size);
}

int generic_ubi_update_bl33(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	size_t fip_part_size, new_fip_size, buf_size;
	struct ubi_volume *vol;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = ubi_mount_default();
	if (ret)
		return ret;

	vol = ubi_find_volume(PART_FIP_NAME);
	if (!vol) {
		cprintln(ERROR, "*** FIP volume not found ***");
		return -ENODEV;
	}

	fip_part_size = (size_t)vol->used_bytes;
	if (fip_part_size > buf_size) {
		cprintln(ERROR, "*** FIP data is too big ***");
		return -E2BIG;
	}

	ret = ubi_volume_read(PART_FIP_NAME, buf, 0, fip_part_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	ret = fip_update_uboot_data(data, size, buf, fip_part_size,
				    &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return ubi_write_fip(vol, PART_FIP_NAME, buf, new_fip_size);
}
#endif

int generic_mtd_update_bl31(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	size_t fip_part_size;
	size_t new_fip_size;
	size_t buf_size;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = read_mtd_part(PART_FIP_NAME, buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}
	ret = fip_update_bl31_data(data, size, buf, fip_part_size,
				   &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_mtd_part(PART_FIP_NAME, buf, new_fip_size, true);
}

int generic_mtd_update_bl33(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	size_t fip_part_size;
	size_t new_fip_size;
	size_t buf_size;
	void *buf;
	int ret;

	ret = get_fip_buffer(FIP_READ_BUFFER, &buf, &buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP buffer failed (%d) ***", ret);
		return -ENOBUFS;
	}

	ret = read_mtd_part(PART_FIP_NAME, buf, &fip_part_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP read failed (%d) ***", ret);
		return -EBADMSG;
	}

	ret = fip_update_uboot_data(data, size, buf, fip_part_size,
				    &new_fip_size, buf_size);
	if (ret) {
		cprintln(ERROR, "*** FIP update u-boot failed (%d) ***", ret);
		return -EBADMSG;
	}

	return write_mtd_part(PART_FIP_NAME, buf, new_fip_size, true);
}

int generic_mtd_write_fw(void *priv, const struct data_part_entry *dpe,
			 const void *data, size_t size)
{
	int ret = 0;

	ret = mtd_upgrade_image(data, size);
	if (ret)
		cprintln(ERROR, "*** Image not supported! ***");

	return ret;
}

int generic_mtd_write_simg(void *priv, const struct data_part_entry *dpe,
			   const void *data, size_t size)
{
	struct mtd_info *mtd, *mtd_nmbm = NULL;
	uint32_t i;
	int ret;

#ifdef CONFIG_ENABLE_NAND_NMBM
	mtd_nmbm = get_mtd_device_nm("nmbm0");
	if (IS_ERR(mtd_nmbm))
		mtd_nmbm = NULL;
	else
		put_mtd_device(mtd_nmbm);
#endif

	for (i = 0; i < 64; i++) {
		mtd = get_mtd_device(NULL, i);
		if (!IS_ERR(mtd)) {
			if (mtd_nmbm && mtd == mtd_nmbm)
				continue;
			if (mtd->parent)
				continue;
			break;
		}
	}

	if (IS_ERR(mtd))
		return -PTR_ERR(mtd);

	ret = mtd_erase_skip_bad(mtd, 0, mtd->size, mtd->size, NULL, NULL,
				 NULL, false);
	if (ret) {
		put_mtd_device(mtd);
		return ret;
	}

	ret = mtd_write_skip_bad(mtd, 0, size, mtd->size, NULL, data, true);

	put_mtd_device(mtd);

	return ret;
}

int generic_mtd_validate_fw(void *priv, const struct data_part_entry *dpe,
			    const void *data, size_t size)
{
	struct owrt_image_info ii;
	bool rc, verify_rootfs;
	struct mtd_info *mtd;

	if (IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_VERIFY)) {
		mtd = get_mtd_part(NULL);
		if (IS_ERR(mtd))
			return -PTR_ERR(mtd);

		put_mtd_device(mtd);

		verify_rootfs = IS_ENABLED(CONFIG_MTK_UPGRADE_IMAGE_ROOTFS_VERIFY);

		rc = verify_image_ram(data, size, mtd->erasesize,
				      verify_rootfs, &ii, NULL, NULL);
		if (!rc) {
			cprintln(ERROR, "*** Firmware integrity verification failed ***");
			return -EBADMSG;
		}
	}

	return 0;
}
