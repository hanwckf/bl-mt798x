// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic A/B boot implementation
 */

#include <env.h>
#include <stdio.h>
#include <linux/errno.h>
#include <vsprintf.h>
#include <image.h>
#include <part.h>
#include <mmc.h>

#include "verify_helper.h"
#include "dual_boot.h"
#include "boot_helper.h"
#include "upgrade_helper.h"

#define BOOT_PARAM_STR_MAX_LEN			256

static struct bootarg bootargs[20];	/* at least 11 */
static u32 num_bootargs;

static char boot_image_slot[16];
static char boot_kernel_part[BOOT_PARAM_STR_MAX_LEN];
static char boot_rootfs_part[BOOT_PARAM_STR_MAX_LEN];
static char upgrade_image_slot[16];
static char upgrade_kernel_part[BOOT_PARAM_STR_MAX_LEN];
static char upgrade_rootfs_part[BOOT_PARAM_STR_MAX_LEN];
static char env_part[BOOT_PARAM_STR_MAX_LEN];
static char rootfs_data_part[BOOT_PARAM_STR_MAX_LEN];

u32 dual_boot_get_current_slot(void)
{
	char *end, *slot_str = env_get("dual_boot.current_slot");
	ulong slot;

	if (!slot_str)
		return 0;

	slot = simple_strtoul(slot_str, &end, 10);
	if (end == slot_str || *end) {
		printf("Invalid image slot number '%s', default to 0\n",
		       slot_str);
		return 0;
	}

	return slot;
}

u32 dual_boot_get_next_slot(void)
{
	u32 current_slot = dual_boot_get_current_slot();

	return (current_slot + 1) % DUAL_BOOT_MAX_SLOTS;
}

int dual_boot_set_current_slot(u32 slot)
{
	int ret;

	if (slot >= DUAL_BOOT_MAX_SLOTS) {
		printf("Invalid image slot number %u\n", slot);
		return -EINVAL;
	}

	ret = env_set_ulong("dual_boot.current_slot", slot);
	if (ret) {
		printf("Failed to set current slot in env\n");
		return ret;
	}

	ret = env_save();
	if (ret)
		printf("Failed to save env\n");

	return ret;
}

static void bootargs_reset(void)
{
	num_bootargs = 0;
}

static int bootargs_set(const char *key, const char *value)
{
	if (num_bootargs >= ARRAY_SIZE(bootargs)) {
		panic("Error: No space for bootargs\n");
		return -1;
	}

	bootargs[num_bootargs].key = key;
	bootargs[num_bootargs].value = value;
	num_bootargs++;

	return 0;
}

void board_prep_linux(bootm_headers_t *images)
{
	void *fdt = images->ft_addr;
	const char *orig_bootargs;
	int nodeoffset, len;
	char *new_cmdline;
	u32 slot, newlen;
	int ret;

	if (!(CONFIG_IS_ENABLED(OF_LIBFDT) && images->ft_len)) {
		printf("Warning: no FDT present for current image!\n");
		return;
	}

	/* find or create "/chosen" node. */
	nodeoffset = fdt_find_or_add_subnode(fdt, 0, "chosen");
	if (nodeoffset < 0)
		return;

	orig_bootargs = fdt_getprop(fdt, nodeoffset, "bootargs", &len);

	debug("%s: orig cmdline = \"%s\"\n", __func__, orig_bootargs);

	/* setup bootargs */
	slot = dual_boot_get_current_slot();
	snprintf(boot_image_slot, sizeof(boot_image_slot), "%u", slot);
	bootargs_set("boot_param.boot_image_slot", boot_image_slot);

	slot = dual_boot_get_next_slot();
	snprintf(upgrade_image_slot, sizeof(upgrade_image_slot), "%u", slot);
	bootargs_set("boot_param.upgrade_image_slot", upgrade_image_slot);

	bootargs_set("boot_param.dual_boot", NULL);

	ret = cmdline_merge(orig_bootargs, bootargs, num_bootargs, &new_cmdline);
	if (!ret) {
		panic("Error: failed to generate new kernel cmdline\n");
		return;
	}

	debug("%s: new cmdline = \"%s\"\n", __func__, new_cmdline);

	newlen = strlen(new_cmdline) + 1;
	fdt_shrink_to_minimum(fdt, newlen);

	ret = fdt_setprop(fdt, nodeoffset, "bootargs", new_cmdline, newlen);
	if (ret < 0) {
		panic("Error: failed to set new kernel cmdline\n");
		return;
	}
}

#ifdef CONFIG_MMC
static int mmc_fill_partuuid(struct mmc *mmc, const char *partname, char *var)
{
	struct disk_partition dpart;
	int ret;

	ret = _mmc_find_part(mmc, partname, &dpart);
	if (ret)
		return -ENODEV;

	debug("part '%s' uuid = %s\n", partname, dpart.uuid);

	snprintf(var, BOOT_PARAM_STR_MAX_LEN, "PARTUUID=%s", dpart.uuid);

	return 0;
}

static int mmc_set_bootarg_part(struct mmc *mmc, const char *partname,
				const char *argname, char *argval)
{
	int ret;

	ret = mmc_fill_partuuid(mmc, partname, argval);
	if (ret) {
		panic("Error: failed to find part named '%s'\n", partname);
		return ret;
	}

	return bootargs_set(argname, argval);
}

static int mmc_set_bootargs(struct mmc_boot_data *mbd)
{
	struct mmc *mmc;
	u32 slot;
	int ret;

	bootargs_reset();

	ret = bootargs_set("boot_param.no_split_rootfs_data", NULL);
	if (ret)
		return ret;

	mmc = _mmc_get_dev(mbd->dev, 0, false);
	if (!mmc)
		return -ENODEV;

	slot = dual_boot_get_current_slot();

	ret = mmc_set_bootarg_part(mmc, mbd->boot_slots[slot].kernel,
				   "boot_param.boot_kernel_part",
				   boot_kernel_part);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, mbd->boot_slots[slot].rootfs,
				   "boot_param.boot_rootfs_part",
				   boot_rootfs_part);
	if (ret)
		return ret;

	slot = dual_boot_get_next_slot();

	ret = mmc_set_bootarg_part(mmc, mbd->boot_slots[slot].kernel,
				   "boot_param.upgrade_kernel_part",
				   upgrade_kernel_part);
	if (ret)
		return ret;

	ret = mmc_set_bootarg_part(mmc, mbd->boot_slots[slot].rootfs,
				   "boot_param.upgrade_rootfs_part",
				   upgrade_rootfs_part);
	if (ret)
		return ret;

	if (mbd->env_part) {
		ret = mmc_set_bootarg_part(mmc, mbd->env_part,
					   "boot_param.env_part", env_part);
		if (ret)
			return ret;
	}

	if (mbd->rootfs_data_part) {
		ret = mmc_set_bootarg_part(mmc, mbd->rootfs_data_part,
					   "boot_param.rootfs_data_part",
					   rootfs_data_part);
		if (ret)
			return ret;
	}

	return bootargs_set("root", boot_rootfs_part);
}

#ifdef CONFIG_MTK_DUAL_BOOT_IMAGE_ROOTFS_VERIFY
static int mmc_boot_slot(struct mmc_boot_data *mbd, u32 slot)
{
	struct mmc_verify_data vd = { 0 };
	ulong data_load_addr;
	int ret;

	printf("Trying to boot from image slot %u\n", slot);

	vd.dev = mbd->dev;
	vd.kernel_part = mbd->boot_slots[slot].kernel;
	vd.rootfs_part = mbd->boot_slots[slot].rootfs;
	vd.kernel_data = mbd->load_ptr;

	ret = mmc_boot_verify(&vd);
	if (ret) {
		printf("Firmware integrity verification failed\n");
		return ret;
	}

	printf("Firmware integrity verification passed\n");

	/* Move kernel image to load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
	data_load_addr = CONFIG_SYS_LOAD_ADDR;
#elif defined(CONFIG_LOADADDR)
	data_load_addr = CONFIG_LOADADDR;
#endif
	memmove((void *)data_load_addr, vd.kernel_data, vd.kernel_size);

	/* Boot current image */
	return boot_from_mem(data_load_addr);
}
#else
static int mmc_boot_slot(struct mmc_boot_data *mbd, u32 slot)
{
	printf("Trying to boot from image slot %u\n", slot);

	return boot_from_mmc_partition(mbd->dev, 0,
				       mbd->boot_slots[slot].kernel);
}
#endif /* CONFIG_MTK_DUAL_BOOT_IMAGE_ROOTFS_VERIFY */

int dual_boot_mmc(struct mmc_boot_data *mbd)
{
	u32 slot;
	int ret;

	mmc_set_bootargs(mbd);

	slot = dual_boot_get_current_slot();
	ret = mmc_boot_slot(mbd, slot);

	printf("Failed to boot from current image slot, error %d\n", ret);

	slot = dual_boot_get_next_slot();
	ret = dual_boot_set_current_slot(slot);
	if (ret) {
		panic("Error: failed to set new image boot slot, error %d\n",
		       ret);
		return ret;
	}

	mmc_set_bootargs(mbd);

	ret = mmc_boot_slot(mbd, slot);

	printf("Failed to boot from next image slot, error %d\n", ret);

	return ret;
}
#endif /* CONFIG_MMC */
