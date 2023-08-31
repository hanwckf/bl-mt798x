// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Failsafe operations
 */

#include <command.h>
#include <errno.h>
#include <linux/string.h>
#include <asm/global_data.h>
#include "upgrade_helper.h"
#include "colored_print.h"

DECLARE_GLOBAL_DATA_PTR;

#define UPGRADE_PART		"fw"

static const struct data_part_entry *find_part(const struct data_part_entry *parts,
					       u32 num_parts, const char *abbr)
{
	u32 i;

	if (!abbr)
		return NULL;

	for (i = 0; i < num_parts; i++) {
		if (!strcmp(parts[i].abbr, abbr))
			return &parts[i];
	}

	cprintln(ERROR, "*** Invalid upgrading part! ***");

	return NULL;
}

void *httpd_get_upload_buffer_ptr(size_t size)
{
	/* Skip BL31 address range started from 0x43000000 */
	return (void *)gd->ram_base + 0x4000000;
}

int failsafe_validate_image(const void *data, size_t size)
{
	const struct data_part_entry *upgrade_parts, *dpe;
	u32 num_parts;

	board_upgrade_data_parts(&upgrade_parts, &num_parts);

	if (!upgrade_parts || !num_parts) {
		printf("mtkupgrade is not configured!\n");
		return -ENOSYS;
	}

	dpe = find_part(upgrade_parts, num_parts, UPGRADE_PART);
	if (!dpe)
		return -ENODEV;

	if (dpe->validate)
		return dpe->validate(dpe->priv, dpe, data, size);

	return 0;
}

int failsafe_write_image(const void *data, size_t size)
{
	const struct data_part_entry *upgrade_parts, *dpe;
	u32 num_parts;
	int ret;

	board_upgrade_data_parts(&upgrade_parts, &num_parts);

	if (!upgrade_parts || !num_parts) {
		printf("mtkupgrade is not configured!\n");
		return -ENOSYS;
	}

	dpe = find_part(upgrade_parts, num_parts, UPGRADE_PART);
	if (!dpe)
		return -ENODEV;

	printf("\n");
	cprintln(PROMPT, "*** Upgrading %s ***", dpe->name);
	cprintln(PROMPT, "*** Data: %zd (0x%zx) bytes at 0x%08lx ***",
		 size, size, (ulong)data);
	printf("\n");

	ret = dpe->write(dpe->priv, dpe, data, size);
	if (ret)
		return ret;

	printf("\n");
	cprintln(PROMPT, "*** %s upgrade completed! ***", dpe->name);
	printf("\n");

	if (dpe->do_post_action)
		dpe->do_post_action(dpe->priv, dpe, data, size);

	return 0;
}
