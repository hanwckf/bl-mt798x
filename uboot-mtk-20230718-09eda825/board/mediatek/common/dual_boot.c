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

#include "dual_boot.h"
#include "boot_helper.h"

#define BOOTARGS_INCR				20

static struct bootarg *bootargs;
static u32 bootargs_used;
static u32 bootargs_max;

static char boot_image_slot[16];
static char upgrade_image_slot[16];

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

static int bootargs_expand(void)
{
	struct bootarg *newptr;

	if (!bootargs) {
		bootargs = malloc(BOOTARGS_INCR * sizeof(*bootargs));
		if (!bootargs)
			return -ENOMEM;

		bootargs_used = 0;
		bootargs_max = BOOTARGS_INCR;
		return 0;
	}

	newptr = realloc(bootargs, (bootargs_max + BOOTARGS_INCR) * sizeof(*bootargs));
	if (!newptr)
		return -ENOMEM;

	bootargs = newptr;
	bootargs_max += BOOTARGS_INCR;
	return 0;
}

void bootargs_reset(void)
{
	bootargs_used = 0;
}

int bootargs_set(const char *key, const char *value)
{
	if (bootargs_used == bootargs_max) {
		if (bootargs_expand()) {
			panic("Error: No space for bootargs\n");
			return -1;
		}
	}

	bootargs[bootargs_used].key = key;
	bootargs[bootargs_used].value = value;
	bootargs_used++;

	return 0;
}

void board_prep_linux(struct bootm_headers *images)
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

	ret = cmdline_merge(orig_bootargs, bootargs, bootargs_used, &new_cmdline);
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
