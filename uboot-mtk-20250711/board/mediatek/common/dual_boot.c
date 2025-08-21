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
#include <malloc.h>
#include <image.h>
#ifdef CONFIG_ARCH_MEDIATEK
#include <linux/arm-smccc.h>
#endif

#include "dual_boot.h"
#include "boot_helper.h"
#include "rootdisk.h"

#define MTK_SIP_READ_NONRST_REG			0xC2000570
#define MTK_SIP_WRITE_NONRST_REG		0xC2000571

const struct dual_boot_slot dual_boot_slots[DUAL_BOOT_MAX_SLOTS] = {
	{
#ifdef CONFIG_MTK_DUAL_BOOT_ITB_IMAGE
		.kernel = PART_FIRMWARE_NAME,
#ifdef CONFIG_MTK_DUAL_BOOT_SLOT_1_ROOTFS_DATA_NAME
		.rootfs_data = PART_ROOTFS_DATA_NAME,
#endif
#else
		.kernel = PART_KERNEL_NAME,
		.rootfs = PART_ROOTFS_NAME,
#endif
	},
#ifdef CONFIG_MTK_DUAL_BOOT
	{
#ifdef CONFIG_MTK_DUAL_BOOT_ITB_IMAGE
		.kernel = CONFIG_MTK_DUAL_BOOT_SLOT_1_FIRMWARE_NAME,
#ifdef CONFIG_MTK_DUAL_BOOT_SLOT_1_ROOTFS_DATA_NAME
		.rootfs_data = CONFIG_MTK_DUAL_BOOT_SLOT_1_ROOTFS_DATA_NAME,
#endif
#else
		.kernel = CONFIG_MTK_DUAL_BOOT_SLOT_1_KERNEL_NAME,
		.rootfs = CONFIG_MTK_DUAL_BOOT_SLOT_1_ROOTFS_NAME,
#endif
	},
#endif /* CONFIG_MTK_DUAL_BOOT */
};

static bool dual_boot_disabled;

static char boot_image_slot[16];
static char upgrade_image_slot[16];

void dual_boot_disable(void)
{
	dual_boot_disabled = true;
}

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

bool dual_boot_is_slot_invalid(u32 slot)
{
	char envname[64], *val;

	snprintf(envname, sizeof(envname), "dual_boot.slot_%u_invalid", slot);

	val = env_get(envname);
	if (!val)
		return false;

	if (strcmp(val, "1"))
		return false;

	return true;
}

int dual_boot_set_slot_invalid(u32 slot, bool invalid, bool save)
{
	char envname[64];
	int ret;

	snprintf(envname, sizeof(envname), "dual_boot.slot_%u_invalid", slot);

	ret = env_set_ulong(envname, invalid ? 1 : 0);
	if (ret) {
		printf("Failed to set image slot %u %s in env\n", slot,
		       invalid ? "invalid" : "valid");
		return ret;
	}

	if (save) {
		ret = env_save();
		if (ret)
			printf("Failed to save env\n");
	}

	return ret;
}

bool dual_boot_get_boot_count(u32 *retslot, u32 *retcnt)
{
#ifdef CONFIG_ARCH_MEDIATEK
	struct arm_smccc_res res = {0};
	u32 val, slot;
	s8 neg, pos;

	arm_smccc_smc(MTK_SIP_READ_NONRST_REG, 0, 0, 0, 0, 0, 0, 0, &res);

	val = (u32)res.a0;

	pr_debug("read boot count: 0x%08x\n", val);

	/* slot: val[31..24] = -slot, val[23..16] = slot */
	pos = (val >> 16) & 0xff;
	neg = (val >> 24) & 0xff;

	if (!(pos >= 0 && neg <= 0 && pos + neg == 0)) {
		pr_debug("slot of boot count is invalid\n");
		goto err;
	}

	slot = pos;

	/* count: val[15..8] = -count, val[7..0] = count */
	pos = val & 0xff;
	neg = (val >> 8) & 0xff;

	if (!(pos >= 0 && neg <= 0 && pos + neg == 0)) {
		pr_debug("count of boot count is invalid\n");
		goto err;
	}

	pr_debug("Boot count: %u of slot %u\n", pos, slot);

	if (retslot)
		*retslot = slot;

	if (retcnt)
		*retcnt = pos;

	return true;

err:
#endif

	if (retslot)
		*retslot = 0;

	if (retcnt)
		*retcnt = 0;

	return false;
}

void dual_boot_set_boot_count(u32 slot, u32 count)
{
#ifdef CONFIG_ARCH_MEDIATEK
	struct arm_smccc_res res = {0};
	u32 val;
	s32 neg;

	if (slot > 127 || count > 127)
		return;

	pr_debug("Set boot count: %u of slot %u\n", count, slot);

	neg = -count;
	val = count | ((neg << 8) & 0xff00);

	neg = -slot;
	val = val | ((uint32_t)slot << 16) | ((neg << 24) & 0xff000000);

	pr_debug("write boot count: 0x%08x\n", val);

	arm_smccc_smc(MTK_SIP_WRITE_NONRST_REG, 0, val, 0, 0, 0, 0, 0, &res);
#endif
}

int dual_boot(struct dual_boot_priv *priv, bool do_boot)
{
	u32 slot, bootcount = 0, maxcount = 0;
	int ret;

	slot = dual_boot_get_current_slot();

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ENABLE_RETRY)) {
		u32 last_slot;
		bool bcvalid;

		bcvalid = dual_boot_get_boot_count(&last_slot, &bootcount);

		if (!bcvalid || slot != last_slot) {
#ifdef CONFIG_ARCH_MEDIATEK
			printf("Boot count is invalid. Assuming cold boot\n");
#endif
			bootcount = 0;
		}

		/* Avoid compilation error */
#ifdef CONFIG_MTK_DUAL_BOOT_MAX_RETRY_COUNT
		maxcount = CONFIG_MTK_DUAL_BOOT_MAX_RETRY_COUNT;
#endif

		if (maxcount < 1)
			maxcount = 1;
	}

	if (dual_boot_is_slot_invalid(slot)) {
		printf("Image slot %u was marked invalid.\n", slot);
	} else {
		if (bootcount) {
			printf("Image slot %u has tried booting for %u times.\n",
			       slot, bootcount);
		}

		if (maxcount && bootcount >= maxcount) {
			printf("Maximum retries (%u) exceeded with image slot %u\n",
			       maxcount, slot);
		} else {
			if (maxcount) {
				printf("Setting image slot %u with boot count %u\n",
				       slot, bootcount + 1);
				dual_boot_set_boot_count(slot, bootcount + 1);
			}

			ret = priv->boot_slot(priv, slot, do_boot);
			if (!ret && !do_boot)
				return 0;

			printf("Failed to boot from current image slot, error %d\n",
			       ret);
		}

		printf("Image slot %u will be marked invalid.\n", slot);

		dual_boot_set_slot_invalid(slot, true, true);
	}

	slot = dual_boot_get_next_slot();

	if (dual_boot_is_slot_invalid(slot)) {
		printf("Image slot %u was marked invalid.\n", slot);
	} else {
		ret = dual_boot_set_current_slot(slot);
		if (ret) {
			panic("Error: failed to set new image boot slot, error %d\n",
			      ret);
			return ret;
		}

		if (maxcount) {
			printf("Setting image slot %u with boot count 1\n", slot);
			dual_boot_set_boot_count(slot, 1);
		}

		ret = priv->boot_slot(priv, slot, do_boot);
			if (!ret && !do_boot)
				return 0;

		printf("Failed to boot from next image slot, error %d\n", ret);
		printf("Image slot %u will be marked invalid.\n", slot);

		dual_boot_set_slot_invalid(slot, true, true);
	}

	return ret;
}

static int dual_boot_set_default_bootargs(void)
{
	u32 slot;

	slot = dual_boot_get_current_slot();
	snprintf(boot_image_slot, sizeof(boot_image_slot), "%u", slot);
	if (bootargs_set("boot_param.boot_image_slot", boot_image_slot))
		return -1;

	slot = dual_boot_get_next_slot();
	snprintf(upgrade_image_slot, sizeof(upgrade_image_slot), "%u", slot);
	if (bootargs_set("boot_param.upgrade_image_slot", upgrade_image_slot))
		return -1;

	if (bootargs_set("boot_param.dual_boot", NULL))
		return -1;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ENABLE_RETRY)) {
		if (bootargs_set("boot_param.reset_boot_count", NULL))
			return -1;
	}

	return 0;
}

static int dual_boot_set_fdt_defaults(void *fdt)
{
	u32 slot;

	slot = dual_boot_get_current_slot();
	rootdisk_set_fitblk_rootfs(fdt, dual_boot_slots[slot].kernel);

	snprintf(boot_image_slot, sizeof(boot_image_slot), "%u", slot);
	if (fdtargs_set("mediatek,boot-image-slot", boot_image_slot))
		return -1;

	slot = dual_boot_get_next_slot();
	snprintf(upgrade_image_slot, sizeof(upgrade_image_slot), "%u", slot);
	if (fdtargs_set("mediatek,upgrade-image-slot", upgrade_image_slot))
		return -1;

	if (fdtargs_set("mediatek,dual-boot", NULL))
		return -1;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ENABLE_RETRY)) {
		if (fdtargs_set("mediatek,reset-boot-count", NULL))
			return -1;
	}

	return 0;
}

int dual_boot_set_defaults(void *fdt)
{
	int ret;

	if (dual_boot_disabled)
		return 0;

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT_ITB_IMAGE))
		ret = dual_boot_set_fdt_defaults(fdt);
	else
		ret = dual_boot_set_default_bootargs();

	if (ret)
		panic("Error: failed to setup dual boot settings\n");

	return ret;
}
