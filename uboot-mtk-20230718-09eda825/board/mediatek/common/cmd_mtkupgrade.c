// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data upgrading command
 */

#include <command.h>
#include <env.h>
#include <image.h>
#include <linux/types.h>

#include "load_data.h"
#include "colored_print.h"
#include "upgrade_helper.h"

static const struct data_part_entry *upgrade_parts;
static u32 num_parts;

static bool prompt_post_action(const struct data_part_entry *dpe)
{
	if (dpe->post_action == UPGRADE_ACTION_REBOOT)
		return confirm_yes("Reboot after upgrading? (Y/n):");

	if (dpe->post_action == UPGRADE_ACTION_BOOT)
		return confirm_yes("Run image after upgrading? (Y/n):");

	if (dpe->post_action == UPGRADE_ACTION_CUSTOM) {
		if (dpe->custom_action_prompt)
			return confirm_yes(dpe->custom_action_prompt);

		return true;
	}

	return false;
}

static int do_post_action(const struct data_part_entry *dpe, const void *data,
			  size_t size)
{
	int ret;

	if (dpe->do_post_action) {
		ret = dpe->do_post_action(dpe->priv, dpe, data, size);

		if (dpe->post_action == UPGRADE_ACTION_CUSTOM)
			return ret;
	}

	if (dpe->post_action == UPGRADE_ACTION_REBOOT) {
		printf("Rebooting ...\n\n");
		return run_command("reset", 0);
	}

	if (dpe->post_action == UPGRADE_ACTION_BOOT)
		return run_command("mtkboardboot", 0);

	return CMD_RET_SUCCESS;
}

static const struct data_part_entry *select_part(void)
{
	u32 i;
	char c;

	printf("\n");
	cprintln(PROMPT, "Available parts to be upgraded:");

	for (i = 0; i < num_parts; i++)
		printf("    %d - %s\n", i, upgrade_parts[i].name);

	while (1) {
		printf("\n");
		cprint(PROMPT, "Select a part:");
		printf(" ");

		c = getchar();
		if (c == '\r' || c == '\n')
			continue;

		printf("%c\n", c);
		break;
	}

	i = c - '0';
	if (c < '0' || i >= num_parts) {
		cprintln(ERROR, "*** Invalid selection! ***");
		return NULL;
	}

	return &upgrade_parts[i];
}

static const struct data_part_entry *find_part(const char *abbr)
{
	u32 i;

	if (!abbr)
		return NULL;

	for (i = 0; i < num_parts; i++) {
		if (!strcmp(upgrade_parts[i].abbr, abbr))
			return &upgrade_parts[i];
	}

	cprintln(ERROR, "*** Invalid upgrading part! ***");

	return NULL;
}

static int do_mtkupgrade(struct cmd_tbl *cmdtp, int flag, int argc,
			 char *const argv[])
{
	const struct data_part_entry *dpe = NULL;
	ulong data_load_addr;
	size_t data_size = 0;
	bool do_action;

	board_upgrade_data_parts(&upgrade_parts, &num_parts);

	if (!upgrade_parts || !num_parts) {
		printf("mtkupgrade is not configured!\n");
		return CMD_RET_FAILURE;
	}

	if (argc < 2)
		dpe = select_part();
	else
		dpe = find_part(argv[1]);

	if (!dpe)
		return CMD_RET_FAILURE;

	printf("\n");
	cprintln(PROMPT, "*** Upgrading %s ***", dpe->name);
	printf("\n");

	do_action = prompt_post_action(dpe);

	/* Set load address */
#if defined(CONFIG_SYS_LOAD_ADDR)
	data_load_addr = CONFIG_SYS_LOAD_ADDR;
#elif defined(CONFIG_LOADADDR)
	data_load_addr = CONFIG_LOADADDR;
#endif

	/* Load data */
	if (load_data(data_load_addr, &data_size, dpe->env_name))
		return CMD_RET_FAILURE;

	printf("\n");
	cprintln(PROMPT, "*** Loaded %zd (0x%zx) bytes at 0x%08lx ***",
		 data_size, data_size, data_load_addr);
	printf("\n");

	image_load_addr = data_load_addr;

	/* Validate data */
	if (dpe->validate) {
		if (dpe->validate(dpe->priv, dpe, (void *)data_load_addr,
				  data_size))
			return CMD_RET_FAILURE;
	}

	/* Write data */
	if (dpe->write(dpe->priv, dpe, (void *)data_load_addr, data_size))
		return CMD_RET_FAILURE;

	printf("\n");
	cprintln(PROMPT, "*** %s upgrade completed! ***", dpe->name);

	if (do_action) {
		puts("\n");
		return do_post_action(dpe, (void *)data_load_addr, data_size);
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(mtkupgrade, 2, 0, do_mtkupgrade,
	   "MTK firmware/bootloader upgrading utility",
	   "mtkupgrade [<part>]\n"
	   "part    - upgrade data part\n"
);
