// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Command for setting bootconf for FIT image
 */

#include <ansi.h>
#include <cli.h>
#include <command.h>
#include <env.h>
#include <image.h>
#include <malloc.h>
#include <menu.h>
#include <watchdog.h>
#include <linux/types.h>
#include <linux/delay.h>

#include "colored_print.h"
#include "boot_helper.h"
#include "load_data.h"

enum fit_conf_type {
	FIT_CONF_UNKNOWN,
	FIT_CONF_KERNEL,
	FIT_CONF_DT_OVERLAY,
};

struct fit_conf_item {
	enum fit_conf_type type;
	const char *name;
	const char *desc;
	int group;
	bool selected;
};

struct fit_conf {
	uint32_t count;
	struct fit_conf_item *items;
};

enum bootconf_menu_item_type {
	BC_MENU_DEFAULT,
	BC_MENU_FINISH,
	BC_MENU_DISCARD,
	BC_MENU_RESET,
	BC_MENU_CLEAR,

	__MAX_BC_MENU_TYPE
};

struct bootconf_menu_item {
	enum bootconf_menu_item_type type;
	struct bootconf_ctx *ctx;
	struct fit_conf_item *item;
};

struct bootconf_menu_ctx {
	uint32_t active;
	uint32_t last_active;

	uint32_t count;
	struct bootconf_menu_item *items;
};

struct bootconf_item_list {
	uint32_t count;
	uint32_t *index;
};

struct bootconf_ctx {
	struct fit_conf fc;
	struct bootconf_menu_ctx menu;
	struct bootconf_item_list selected;
	struct bootconf_item_list selected_extra;
	uint32_t *index_tmp;
};

static const char *const bootconf_menu_item_name[] = {
	[BC_MENU_FINISH] = "!/finish",
	[BC_MENU_DISCARD] = "!/discard",
	[BC_MENU_RESET] = "!/reset",
	[BC_MENU_CLEAR] = "!/clear",
};

int __weak mtk_board_get_fit_conf_info(const char *name, const char **retdesc)
{
	(*retdesc) = NULL;

	return -1;
}

static int load_fit_conf_items(void *fit, struct fit_conf_item *items)
{
	int conf_noffset, node, len;
	uint32_t i = 0, count = 0;
	const char *conf_name;

	conf_noffset = fdt_path_offset(fit, FIT_CONFS_PATH);
	if (conf_noffset < 0) {
		cprintln(ERROR, "Cannot find " FIT_CONFS_PATH " node: %d",
			 conf_noffset);
		return -EINVAL;
	}

	/* Count all configurations */
	for (node = fdt_first_subnode(fit, conf_noffset);
	     node >= 0;
	     node = fdt_next_subnode(fit, node)) {
		count++;
	}

	if (!items)
		return count;

	for (node = fdt_first_subnode(fit, conf_noffset);
	     node >= 0;
	     node = fdt_next_subnode(fit, node)) {
		conf_name = fdt_get_name(fit, node, NULL);
		if (!conf_name)
			continue;

		if (fdt_getprop(fit, node, FIT_KERNEL_PROP, &len))
			items[i].type = FIT_CONF_KERNEL;
		else if (fdt_getprop(fit, node, FIT_FDT_PROP, &len))
			items[i].type = FIT_CONF_DT_OVERLAY;

		items[i].name = conf_name;

		items[i].group = mtk_board_get_fit_conf_info(conf_name,
							     &items[i].desc);

		i++;
	}

	return i;
}

static int find_fit_conf(struct fit_conf *fc, const char *name, ssize_t len)
{
	size_t namelen;
	uint32_t i;

	if (len < 0)
		len = strlen(name);

	for (i = 0; i < fc->count; i++) {
		namelen = strlen(fc->items[i].name);
		if (namelen != len)
			continue;

		if (!strncmp(fc->items[i].name, name, len))
			return i;
	}

	return -1;
}

static void print_bootconf(const char *title, const struct fit_conf *fc,
			   const struct bootconf_item_list *list)
{
	uint32_t i;

	puts(ANSI_CLEAR_LINE);
	cprint(INPUT, "%s", title);
	printf(": ");

	for (i = 0; i < list->count; i++) {
		printf("%s", fc->items[list->index[i]].name);
		if (i < list->count - 1)
			printf("#");
	}

	printf("\n");
}

static void mtkbootconf_display_statusline(struct menu *m)
{
	struct bootconf_menu_item *entry;
	struct bootconf_ctx *ctx;

	if (menu_default_choice(m, (void *)&entry) < 0)
		return;

	ctx = entry->ctx;

	printf(ANSI_CURSOR_POSITION, 1, 1);
	puts(ANSI_CLEAR_LINE);

	puts(ANSI_CLEAR_LINE);
	printf("\t\t*** MediaTek Image Boot Configuration ***\n");

	puts(ANSI_CLEAR_LINE);
	printf("\n");

	print_bootconf("bootconf", &ctx->fc, &ctx->selected);
	print_bootconf("bootconf_extra", &ctx->fc, &ctx->selected_extra);

	puts(ANSI_CLEAR_LINE);
	printf("\n");

	puts(ANSI_CLEAR_LINE);
	printf("Press UP/DOWN to move, ENTER to select\n");

	puts(ANSI_CLEAR_LINE);
	printf("\n");
}

static void mtkbootconf_print_entry(void *data)
{
	struct bootconf_menu_item *entry = data;
	struct bootconf_ctx *ctx = entry->ctx;
	int reverse = (ctx->menu.active == entry - ctx->menu.items);

	puts(ANSI_CLEAR_LINE);

	if (reverse)
		puts(ANSI_COLOR_REVERSE);

	switch (entry->type) {
	case BC_MENU_DEFAULT:
		if (entry->item->type == FIT_CONF_KERNEL) {
			printf("\tSet boot image to %s", entry->item->name);
		} else {
			if (entry->item->group < 0)
				printf("\tAppend %s", entry->item->name);
			else
				printf("\tSelect %s", entry->item->name);
		}

		if (entry->item->desc)
			printf(" (%s)", entry->item->desc);

		printf("\n");
		break;

	case BC_MENU_FINISH:
		printf("\t%s\n", "Save changes & Quit");
		break;

	case BC_MENU_DISCARD:
		printf("\t%s\n", "Discard changes & Quit");
		break;

	case BC_MENU_RESET:
		printf("\t%s\n", "Reset to default");
		break;

	case BC_MENU_CLEAR:
		printf("\t%s\n", "Clear all");
		break;

	default:
		;
	}

	if (reverse)
		puts(ANSI_COLOR_RESET);
}

static enum bootmenu_key mtkbootconf_menu_loop(struct cli_ch_state *cch)
{
	enum bootmenu_key key;
	int c, errchar = 0;

	c = cli_ch_process(cch, 0);
	if (!c) {
		while (!c && !tstc()) {
			schedule();
			mdelay(10);
			c = cli_ch_process(cch, errchar);
			errchar = -ETIMEDOUT;
		}
		if (!c) {
			c = getchar();
			c = cli_ch_process(cch, c);
		}
	}

	key = bootmenu_conv_key(c);

	return key;
}

static char *mtkbootconf_choice_entry(void *data)
{
	struct cli_ch_state s_cch, *cch = &s_cch;
	struct bootconf_ctx *ctx = data;
	struct bootconf_menu_item *entry;
	enum bootmenu_key key;

	cli_ch_init(cch);

	while (1) {
		key = mtkbootconf_menu_loop(cch);

		switch (key) {
		case BKEY_UP:
			ctx->menu.last_active = ctx->menu.active;
			if (ctx->menu.active > 0)
				--ctx->menu.active;
			/* no menu key selected, regenerate menu */
			return NULL;

		case BKEY_DOWN:
			ctx->menu.last_active = ctx->menu.active;
			if (ctx->menu.active < ctx->menu.count - 1)
				++ctx->menu.active;
			/* no menu key selected, regenerate menu */
			return NULL;

		case BKEY_SELECT:
			entry = &ctx->menu.items[ctx->menu.active];

			switch (entry->type) {
			case BC_MENU_DEFAULT:
				return (char *)entry->item->name;

			case BC_MENU_FINISH:
			case BC_MENU_DISCARD:
			case BC_MENU_RESET:
			case BC_MENU_CLEAR:
				return (char *)bootconf_menu_item_name[entry->type];

			default:
				return NULL;
			}

		default:
			break;
		}
	}

	return NULL;
}

static bool mtkbootconf_need_reprint(void *data)
{
	struct bootconf_ctx *ctx = data;
	bool need_reprint;

	need_reprint = ctx->menu.last_active != ctx->menu.active;
	ctx->menu.last_active = ctx->menu.active;

	return need_reprint;
}

static void fill_bootconf_list(struct bootconf_item_list *list,
			       struct fit_conf *fc, const char *bootconf)
{
	const char *curr, *next;
	size_t len;
	int idx;

	list->count = 0;

	if (!bootconf || !bootconf[0])
		return;

	curr = bootconf;

	while (curr) {
		next = strchr(curr, '#');

		if (next) {
			len = next - curr;
			next++;
		} else {
			len = strlen(curr);
		}

		if (curr[0]) {
			idx = find_fit_conf(fc, curr, len);
			if (idx >= 0) {
				list->index[list->count++] = idx;
				fc->items[idx].selected = true;
			}
		}

		curr = next;
	}
}

static void clear_selected_list(struct bootconf_ctx *ctx)
{
	uint32_t i;

	for (i = 0; i < ctx->fc.count; i++)
		ctx->fc.items[i].selected = false;

	ctx->selected.count = 0;
	ctx->selected_extra.count = 0;
}

static void init_selected_list(struct bootconf_ctx *ctx)
{
	const char *s;

	clear_selected_list(ctx);

	s = env_get("bootconf");
	fill_bootconf_list(&ctx->selected, &ctx->fc, s);

	s = env_get("bootconf_extra");
	fill_bootconf_list(&ctx->selected_extra, &ctx->fc, s);
}

static void reset_selected_list(struct bootconf_ctx *ctx)
{
	const char *s;

	clear_selected_list(ctx);

	s = env_get_default("bootconf");
	fill_bootconf_list(&ctx->selected, &ctx->fc, s);

	s = env_get_default("bootconf_extra");
	fill_bootconf_list(&ctx->selected_extra, &ctx->fc, s);
}

static int save_selected_list_to_env(struct bootconf_item_list *list,
				     struct fit_conf *fc, const char *envname)
{
	uint32_t i, len = 0;
	const char *s;
	char *str, *p;
	int ret;

	if (!list->count) {
		ret = env_set(envname, NULL);
		if (ret) {
			cprintln(ERROR, "Failed to unset env '%s'\n", envname);
			return -1;
		}

		printf("Unset env '%s'\n", envname);
		return 0;
	}

	for (i = 0; i < list->count; i++)
		len += strlen(fc->items[list->index[i]].name) + 1;

	str = malloc(len);
	if (!str) {
		cprintln(ERROR, "No memory for selected configuration\n");
		return -ENOMEM;
	}

	p = str;

	for (i = 0; i < list->count; i++) {
		s = fc->items[list->index[i]].name;
		len = strlen(s);

		memcpy(p, s, len);
		p += len;

		if (i < list->count - 1)
			*p++ = '#';
		else
			*p = 0;
	}

	ret = env_set(envname, str);
	if (!ret)
		printf("Set env '%s' to \"%s\"\n", envname, str);
	else
		cprintln(ERROR, "Failed to set env '%s'\n", envname);

	free(str);

	if (ret)
		return -1;

	return 0;
}

static int save_selected_bootconf(struct bootconf_ctx *ctx)
{
	int ret;

	ret = save_selected_list_to_env(&ctx->selected, &ctx->fc, "bootconf");
	if (ret)
		return ret;

	ret = save_selected_list_to_env(&ctx->selected_extra, &ctx->fc,
					"bootconf_extra");
	if (ret)
		return ret;

	return env_save();
}

static void deselect_bootconf_kernel(struct bootconf_ctx *ctx,
				     struct bootconf_item_list *list)
{
	uint32_t i, n = 0;

	if (!list->count)
		return;

	for (i = 0; i < list->count; i++) {
		if (ctx->fc.items[list->index[i]].type != FIT_CONF_KERNEL)
			ctx->index_tmp[n++] = list->index[i];
		else
			ctx->fc.items[list->index[i]].selected = false;
	}

	if (list->count == n)
		return;

	list->count = n;

	for (i = 0; i < n; i++)
		list->index[i] = ctx->index_tmp[i];
}

static void select_bootconf_kernel(struct bootconf_ctx *ctx, uint32_t idx)
{
	uint32_t i;

	deselect_bootconf_kernel(ctx, &ctx->selected);
	deselect_bootconf_kernel(ctx, &ctx->selected_extra);

	for (i = ctx->selected.count; i > 0; i--)
		ctx->selected.index[i] = ctx->selected.index[i - 1];

	ctx->selected.index[0] = idx;
	ctx->selected.count++;

	ctx->fc.items[idx].selected = true;
}

static bool replace_bootconf_group(struct bootconf_ctx *ctx,
				   struct bootconf_item_list *list,
				   uint32_t idx, int group)
{
	bool replaced = false;
	uint32_t i, n = 0;

	if (!list->count)
		return false;

	for (i = 0; i < list->count; i++) {
		if (ctx->fc.items[list->index[i]].group != group) {
			ctx->index_tmp[n++] = list->index[i];
		} else {
			ctx->fc.items[list->index[i]].selected = false;

			if (!replaced) {
				ctx->index_tmp[n++] = idx;
				ctx->fc.items[idx].selected = true;
				replaced = true;
			}
		}
	}

	list->count = n;

	for (i = 0; i < n; i++)
		list->index[i] = ctx->index_tmp[i];

	return replaced;
}

static void select_bootconf_dt_overlay(struct bootconf_ctx *ctx, uint32_t idx)
{
	bool rc;

	if (ctx->fc.items[idx].group >= 0) {
		rc = replace_bootconf_group(ctx, &ctx->selected, idx,
					    ctx->fc.items[idx].group);
		if (rc)
			return;

		rc = replace_bootconf_group(ctx, &ctx->selected_extra, idx,
					    ctx->fc.items[idx].group);
		if (rc)
			return;
	}

	if (!ctx->selected.count && !ctx->selected_extra.count)
		ctx->selected.index[ctx->selected.count++] = idx;
	else
		ctx->selected_extra.index[ctx->selected_extra.count++] = idx;

	ctx->fc.items[idx].selected = true;
}

static int select_bootconf(struct bootconf_ctx *ctx, struct fit_conf_item *item)
{
	uint32_t idx = item - ctx->fc.items;

	switch (item->type) {
	case FIT_CONF_KERNEL:
		select_bootconf_kernel(ctx, idx);
		return 0;

	case FIT_CONF_DT_OVERLAY:
		select_bootconf_dt_overlay(ctx, idx);
		return 0;

	default:
		/* should not happen */
		cprintln(ERROR, "Invalid FDT configuration\n");
		return -1;
	}
}

static int start_mtkbootconf(struct bootconf_ctx *ctx)
{
	struct bootconf_menu_item *choice = NULL;
	struct fit_conf *fc = &ctx->fc;
	const char *item_name;
	uint32_t i, nkrnl = 0;
	bool apply = false;
	struct menu *menu;
	int ret;

	menu = menu_create(NULL, 0, 1, mtkbootconf_display_statusline,
			   mtkbootconf_print_entry, mtkbootconf_choice_entry,
			   mtkbootconf_need_reprint, ctx);
	if (!menu) {
		cprintln(ERROR, "Failed to create menu\n");
		return -1;
	}

	ctx->menu.last_active = (uint32_t)-1;
	ctx->menu.active = 0;
	ctx->menu.count = 0;

	for (i = BC_MENU_FINISH; i < __MAX_BC_MENU_TYPE; i++) {
		ctx->menu.items[ctx->menu.count].type = i;
		ctx->menu.items[ctx->menu.count].item = NULL;
		ctx->menu.items[ctx->menu.count].ctx = ctx;
		ctx->menu.count++;
	}

	for (i = 0; i < fc->count; i++) {
		if (fc->items[i].type == FIT_CONF_KERNEL)
			nkrnl++;
	}

	for (i = 0; i < fc->count; i++) {
		if (fc->items[i].selected)
			continue;

		if ((fc->items[i].type == FIT_CONF_DT_OVERLAY) ||
		    (fc->items[i].type == FIT_CONF_KERNEL && nkrnl > 1)) {
			ctx->menu.items[ctx->menu.count].type = BC_MENU_DEFAULT;
			ctx->menu.items[ctx->menu.count].item = &fc->items[i];
			ctx->menu.items[ctx->menu.count].ctx = ctx;
			ctx->menu.count++;
		}
	}

	for (i = 0; i < ctx->menu.count; i++) {
		item_name = bootconf_menu_item_name[ctx->menu.items[i].type];
		if (!item_name)
			item_name = ctx->menu.items[i].item->name;

		if (menu_item_add(menu, (char *)item_name,
				  &ctx->menu.items[i]) != 1) {
			cprintln(ERROR, "Failed to add menu item \"%s\"\n",
				 item_name);
			ret = -1;
			goto cleanup;
		}
	}

	/* Default menu entry is always first */
	menu_default_set(menu, (char *)bootconf_menu_item_name[BC_MENU_FINISH]);

	puts(ANSI_CURSOR_HIDE);
	puts(ANSI_CLEAR_CONSOLE);
	printf(ANSI_CURSOR_POSITION, 1, 1);

	if (menu_get_choice(menu, (void *)&choice) == 1) {
		switch (choice->type) {
		case BC_MENU_FINISH:
			apply = true;
			break;

		case BC_MENU_DISCARD:
			ret = 1;
			break;

		case BC_MENU_RESET:
			reset_selected_list(ctx);
			ret = 0;
			break;

		case BC_MENU_CLEAR:
			clear_selected_list(ctx);
			ret = 0;
			break;

		case BC_MENU_DEFAULT:
			ret = select_bootconf(ctx, choice->item);
			break;

		default:
			/* should not happen */
			cprintln(ERROR, "Invalid menu choice\n");
			ret = -1;
		}
	} else {
		cprintln(ERROR, "Failed to get menu choice\n");
		ret = -1;
	}

	puts(ANSI_CURSOR_SHOW);
	puts(ANSI_CLEAR_CONSOLE);
	printf(ANSI_CURSOR_POSITION, 1, 1);

	if (apply) {
		ret = save_selected_bootconf(ctx);
		if (!ret)
			ret = 1;
	}

cleanup:
	menu_destroy(menu);

	return ret;
}

static int do_mtkbootconf(struct cmd_tbl *cmdtp, int flag, int argc,
			   char *const argv[])
{
	struct bootconf_ctx *ctx;
	int ret, nconf;
	uint32_t len;
	void *fit;

	ret = board_boot_default(false);
	if (ret) {
		cprintln(ERROR, "Failed to load image. Please upgrade valid firmware image first.");
		return ret;
	}

	fit = (void *)get_load_addr();

	nconf = load_fit_conf_items(fit, NULL);
	if (nconf < 0)
		return CMD_RET_FAILURE;

	if (!nconf) {
		cprintln(ERROR, "No valid configuration found in image.\n");
		return CMD_RET_FAILURE;
	}

	len = sizeof(*ctx) +
	      nconf * (sizeof(*ctx->fc.items) +
	      	       3 * sizeof(*ctx->selected.index)) +
	      (nconf + __MAX_BC_MENU_TYPE) * sizeof(*ctx->menu.items);

	ctx = calloc(1, len);
	if (!ctx) {
		cprintln(ERROR, "No memory for bootconf interactive context\n");
		return CMD_RET_FAILURE;
	}

	ctx->fc.items = (struct fit_conf_item *)(ctx + 1);
	ctx->fc.count = load_fit_conf_items(fit, ctx->fc.items);

	ctx->selected.index = (uint32_t *)(ctx->fc.items + nconf);
	ctx->selected_extra.index = ctx->selected.index + nconf;
	ctx->index_tmp = ctx->selected_extra.index + nconf;

	ctx->menu.items = (struct bootconf_menu_item *)(ctx->index_tmp + nconf);

	init_selected_list(ctx);

	do {
		ret = start_mtkbootconf(ctx);
	} while (!ret);

	free(ctx);

	if (ret > 0)
		ret = CMD_RET_SUCCESS;

	return ret;
}

U_BOOT_CMD(mtkbootconf, 1, 0, do_mtkbootconf,
	   "Setup bootconf for FIT image", ""
);
