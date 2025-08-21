// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image boot helper
 */

#include <env.h>
#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/ctype.h>
#include <linux/string.h>

#include "upgrade_helper.h"
#include "image_helper.h"
#include "boot_helper.h"
#include "dual_boot.h"
#include "dm_parser.h"

#define ARGLIST_INCR				20

struct bootconf_list {
	u32 max;
	u32 used;
	char **confs;
};

static struct arg_list bootargs = { .name = "bootargs" };
static struct arg_list fdtargs = { .name = "fdtargs" };

static int bootconf_list_expand(struct bootconf_list *list)
{
	char **newptr;

	if (!list->confs) {
		newptr = malloc(ARGLIST_INCR * sizeof(*list->confs));
		list->used = 0;
		list->max = 0;
	} else {
		newptr = realloc(list->confs,
				 (list->max + ARGLIST_INCR) *
				 sizeof(*list->confs));
	}

	if (!newptr)
		return -ENOMEM;

	list->confs = newptr;
	list->max += ARGLIST_INCR;

	return 0;
}

static int bootconf_list_add(struct bootconf_list *list, const char *conf)
{
	u32 i;

	for (i = 0; i < list->used; i++) {
		if (!strcmp(conf, list->confs[i]))
			return 0;
	}

	if (list->used == list->max) {
		if (bootconf_list_expand(list)) {
			panic("Error: No space for bootconf list\n");
			return -ENOMEM;
		}
	}

	list->confs[list->used] = strdup(conf);
	if (!list->confs[list->used]) {
		panic("Error: No space for bootconf item\n");
		return -ENOMEM;
	}

	list->used++;

	return 0;
}

static void bootconf_list_cleanup(struct bootconf_list *list)
{
	u32 i;

	for (i = 0; i < list->used; i++)
		free(list->confs[i]);

	free(list->confs);

	memset(list, 0, sizeof(*list));
}

static int parse_bootconf(struct bootconf_list *list, const char *bootconf,
			  void *fit)
{
	char *tmp, *curr, *next;
	size_t len;
	int ret;

	if (!bootconf || !bootconf[0])
		return 0;

	tmp = strdup(bootconf);
	curr = tmp;

	while (curr) {
		next = strchr(curr, '#');

		if (next) {
			len = next - curr;
			*next = 0;
			next++;
		} else {
			len = strlen(curr);
		}

		if (curr[0]) {
			if (fit_image_conf_exists(fit, curr)) {
				ret = bootconf_list_add(list, curr);
				if (ret) {
					free(tmp);
					return ret;
				}
			}
		}

		curr = next;
	}

	free(tmp);

	return 0;
}

int boot_from_mem(ulong data_load_addr)
{
	const char *bootconf, *bootconf_extra, *bootconf_stock;
	struct bootconf_list bootconf_list = { 0 };
	bool use_stock_bootconf = false;
	char *cmd = NULL, *p;
	size_t len, i;
	int ret;

	bootconf_stock = fit_image_conf_def((void *)data_load_addr);

	bootconf = env_get("bootconf");
	if (bootconf) {
		ret = parse_bootconf(&bootconf_list, bootconf,
				     (void *)data_load_addr);
		if (ret)
			goto cleanup;
	}

	bootconf_extra = env_get("bootconf_extra");
	if (bootconf_extra) {
		ret = parse_bootconf(&bootconf_list, bootconf_extra,
				     (void *)data_load_addr);
		if (ret)
			goto cleanup;
	}

	if (bootconf_list.used) {
		if (!fit_image_check_kernel_conf((void *)data_load_addr,
		    bootconf_list.confs[0]))
			use_stock_bootconf = true;
	}

	len = 32 /* bootm 0x[1..16]# + NULL */;

	for (i = 0; i < bootconf_list.used; i++)
		len += strlen(bootconf_list.confs[i]) + 1;

	if (use_stock_bootconf)
		len += strlen(bootconf_stock) + 1;

	cmd = calloc(1, len);
	if (!cmd) {
		printf("Error: no memory for boot command\n");
		ret = -ENOMEM;
		goto cleanup;
	}

	p = cmd + snprintf(cmd, len, "bootm 0x%lx", data_load_addr);

	if (use_stock_bootconf)
		p += snprintf(p, len - (p - cmd), "#%s", bootconf_stock);

	for (i = 0; i < bootconf_list.used; i++)
		p += snprintf(p, len - (p - cmd), "#%s", bootconf_list.confs[i]);

	p = strchr(cmd, '#');
	if (p) {
		if (!strcmp(p + 1, bootconf_stock))
			*p = 0;
		else
			printf("bootconf: %s\n", p + 1);
	}

	ret = run_command(cmd, 0);

cleanup:
	bootconf_list_cleanup(&bootconf_list);

	free(cmd);

	return ret;
}

const char *get_arg_next(const char *args, const char **param,
			 size_t *keylen)
{
	unsigned int i, equals = 0;
	int in_quote = 0;

	args = skip_spaces(args);
	if (!*args)
		return NULL;

	if (*args == '"') {
		args++;
		in_quote = 1;
	}

	for (i = 0; args[i]; i++) {
		if (isspace(args[i]) && !in_quote)
			break;

		if (equals == 0) {
			if (args[i] == '=')
				equals = i;
		}

		if (args[i] == '"')
			in_quote = !in_quote;
	}

	*param = args;

	if (equals)
		*keylen = equals;
	else
		*keylen = i;

	return args + i;
}

void list_all_args(const char *args)
{
	const char *n = args, *p;
	size_t len, keylen;

	while (1) {
		n = get_arg_next(n, &p, &keylen);
		if (!n)
			break;

		len = n - p;

		printf("[%zu], <%.*s> \"%.*s\"\n", len, (u32)keylen, p,
		       (u32)len, p);
	}
}

static bool compare_key(const char *key, size_t keylen, const char *str)
{
	size_t ckeylen;

	ckeylen = strlen(str);
	if (ckeylen == keylen) {
		if (!strncmp(str, key, keylen))
			return true;
	}

	return false;
}

static bool bootargs_find(const struct arg_pair *bootargs, u32 count,
			  const char *key, size_t keylen)
{
	u32 i;

	for (i = 0; i < count; i++) {
		if (bootargs[i].key) {
			if (compare_key(key, keylen, bootargs[i].key))
				return true;
		}
	}

	return false;
}

static int arg_list_expand(struct arg_list *arglist)
{
	struct arg_pair *newptr;

	if (!arglist->ap) {
		newptr = malloc(ARGLIST_INCR * sizeof(*arglist->ap));
		arglist->used = 0;
		arglist->max = 0;
	} else {
		newptr = realloc(arglist->ap,
				 (arglist->max + ARGLIST_INCR) *
				 sizeof(*arglist->ap));
	}

	if (!newptr)
		return -ENOMEM;

	arglist->ap = newptr;
	arglist->max += ARGLIST_INCR;

	return 0;
}

static int arg_list_set(struct arg_list *arglist, const char *key,
			const char *value)
{
	if (arglist->used == arglist->max) {
		if (arg_list_expand(arglist)) {
			panic("Error: No space for %s\n", arglist->name);
			return -1;
		}
	}

	arglist->ap[arglist->used].key = key;
	arglist->ap[arglist->used].value = value;
	arglist->used++;

	return 0;
}

static void arg_list_remove(struct arg_list *arglist, const char *key)
{
	u32 i;

	for (i = 0; i < arglist->used; i++) {
		if (strcmp(arglist->ap[i].key, key))
			continue;

		if (i < arglist->used - 1) {
			memmove(&arglist->ap[i], &arglist->ap[i + 1],
				(arglist->used - i - 1) * sizeof(*arglist->ap));
		}

		arglist->used--;
		break;
	}
}

void bootargs_reset(void)
{
	bootargs.used = 0;
}

int bootargs_set(const char *key, const char *value)
{
	return arg_list_set(&bootargs, key, value);
}

void bootargs_unset(const char *key)
{
	arg_list_remove(&bootargs, key);
}

void fdtargs_reset(void)
{
	fdtargs.used = 0;
}

int fdtargs_set(const char *prop, const char *value)
{
	return arg_list_set(&fdtargs, prop, value);
}

void fdtargs_unset(const char *prop)
{
	arg_list_remove(&fdtargs, prop);
}

#ifdef CONFIG_MTK_SECURE_BOOT
static char *strconcat(char *dst, const char *src)
{
	while (*src)
		*dst++ = *src++;

	return dst;
}

static char *strnconcat(char *dst, const char *src, size_t n)
{
	while (n-- && *src)
		*dst++ = *src++;

	return dst;
}
#endif /* CONFIG_MTK_SECURE_BOOT */

static int cmdline_merge(const char *cmdline, const struct arg_pair *bootargs,
			 u32 count, char **result)
{
	size_t cmdline_len = 0, newlen = 0, plen, keylen;
	const char *n = cmdline, *p;
	char *buff, *b;
	bool matched;
	u32 i;

#ifdef CONFIG_MTK_SECURE_BOOT
	const char *rootdev = NULL;
	struct dm_verity_info dvi;
	char *orig_rootdev = NULL;
	char *dm_mod = NULL;
	int ret;
#endif

	if (count && !bootargs)
		return -EINVAL;

	for (i = 0; i < count; i++) {
		if (bootargs[i].key) {
			newlen += strlen(bootargs[i].key);
			if (bootargs[i].value)
				newlen += strlen(bootargs[i].value) + 1;
			newlen++;

#ifdef CONFIG_MTK_SECURE_BOOT
			if (!strcmp(bootargs[i].key, "root") && bootargs[i].value) {
				debug("%s: new rootdev '%s' will be used for dm-verity arg replacing\n",
				       __func__, bootargs[i].value);
				rootdev = bootargs[i].value;
			}
#endif
		}
	}

	if (!newlen)
		return 0;

	if (cmdline)
		cmdline_len = strlen(cmdline);

#ifdef CONFIG_MTK_SECURE_BOOT
	if (rootdev)
		newlen += 2 * strlen(rootdev);
#endif

	buff = malloc(cmdline_len + newlen + 1);
	if (!buff) {
		printf("No memory for new cmdline\n");
		return -ENOMEM;
	}

	b = buff;

	if (cmdline_len) {
		while (1) {
			n = get_arg_next(n, &p, &keylen);
			if (!n)
				break;

			plen = n - p;

			matched = bootargs_find(bootargs, count, p, keylen);

#ifdef CONFIG_MTK_SECURE_BOOT
			if (compare_key(p, keylen, "dm-mod.create")) {
				debug("%s: found dm-mod.create in bootargs, copy and remove it\n",
				       __func__);
				if (p[keylen + 1] == '\"')
					dm_mod = strndup(p + keylen + 2, plen - keylen - 3);
				else
					dm_mod = strndup(p + keylen + 1, plen - keylen - 1);
				matched = true;
			}

			if (compare_key(p, keylen, "root")) {
				debug("%s: found root in bootargs, keep it\n",
				       __func__);
				orig_rootdev = strndup(p + keylen + 1, plen - keylen - 1);
				matched = false;
			}
#endif

			if (matched) {
				debug("%s: matched key '%.*s', removing ...\n",
				      __func__, (u32)keylen, p);
			} else {
				memcpy(b, p, plen);
				b += plen;
				*b++ = ' ';
			}
		}
	}

#ifdef CONFIG_MTK_SECURE_BOOT
	if (!dm_mod && (!orig_rootdev || strncmp(orig_rootdev, "/dev/ram", strlen("/dev/ram")))) {
		panic("Error: dm-mod.create not found in original bootargs!\n");
		free(orig_rootdev);
		return -EINVAL;
	}
	free(orig_rootdev);
#endif

	for (i = 0; i < count; i++) {
		if (bootargs[i].key) {
#ifdef CONFIG_MTK_SECURE_BOOT
			if (dm_mod && rootdev && !strcmp(bootargs[i].key, "root")) {
				debug("%s: skipping adding new root parameter\n",
				       __func__);
				continue;
			}
#endif
			keylen = strlen(bootargs[i].key);
			memcpy(b, bootargs[i].key, keylen);
			b += keylen;

			if (bootargs[i].value) {
				*b++ = '=';

				plen = strlen(bootargs[i].value);
				memcpy(b, bootargs[i].value, plen);
				b += plen;
			}

			*b++ = ' ';
		}
	}

#ifdef CONFIG_MTK_SECURE_BOOT
	if (dm_mod && rootdev) {
		ret = dm_mod_create_arg_parse(dm_mod, "verity", (struct dm_info *)&dvi);
		if (ret < 0) {
			panic("Failed to parse dm-mod.create\n");
			return ret;
		}

		b = strconcat(b, "dm-mod.create=\"");
		b = strnconcat(b, dm_mod, dvi.datadev_pos);
		b = strconcat(b, rootdev);
		b = strnconcat(b, dm_mod + dvi.datadev_pos + dvi.datadev_len,
			       dvi.hashdev_pos - dvi.datadev_pos - dvi.datadev_len);

		if (!strcmp(dvi.datadev, dvi.hashdev))
			b = strconcat(b, rootdev);
		else
			b = strconcat(b, dvi.hashdev);

		b = strconcat(b, dm_mod + dvi.hashdev_pos + dvi.hashdev_len);
		b = strconcat(b, "\" "); /* trailing space is needed */
	} else if (dm_mod && !rootdev) {
		b = strconcat(b, "dm-mod.create=\"");
		b = strnconcat(b, dm_mod, strlen(dm_mod));
		b = strconcat(b, "\" ");
	}
#endif

	if (b > buff)
		b[-1] = 0;

	*result = buff;
	return 1;
}

static int fdt_root_prop_merge(void *fdt)
{
	int ret, np, len;
	u32 i;

	for (i = 0; i < fdtargs.used; i++) {
		np = fdt_path_offset(fdt, "/");
		if (np < 0)
			return -ENOENT;

		if (fdtargs.ap[i].value)
			len = strlen(fdtargs.ap[i].value) + 1;
		else
			len = 0;

		ret = fdt_setprop(fdt, np, fdtargs.ap[i].key,
				  fdtargs.ap[i].value, len);
		if (ret < 0) {
			if (len) {
				printf("Failed to set prop '%s = \"%s\"' in FDT",
				       fdtargs.ap[i].key, fdtargs.ap[i].value);
			} else {
				printf("Failed to set prop '%s' in FDT",
				       fdtargs.ap[i].key);
			}

			return -1;
		}
	}

	return 0;
}

void __weak default_boot_set_defaults(void *fdt)
{
}

void board_prep_linux(struct bootm_headers *images)
{
	void *fdt = images->ft_addr;
	const char *orig_bootargs;
	int nodeoffset, len;
	char *new_cmdline;
	u32 newlen;
	int ret;

	if (!(CONFIG_IS_ENABLED(OF_LIBFDT) && images->ft_len)) {
		printf("Warning: no FDT present for current image!\n");
		return;
	}

	if (IS_ENABLED(CONFIG_MTK_DUAL_BOOT))
		dual_boot_set_defaults(fdt);
	else
		default_boot_set_defaults(fdt);

	if (!bootargs.used && !fdtargs.used)
		return;

	fdt_increase_size(fdt, 0x1000);

	if (bootargs.used) {
		/* find or create "/chosen" node. */
		nodeoffset = fdt_find_or_add_subnode(fdt, 0, "chosen");
		if (nodeoffset < 0)
			return;

		orig_bootargs = fdt_getprop(fdt, nodeoffset, "bootargs", &len);

		debug("%s: orig cmdline = \"%s\"\n", __func__, orig_bootargs);

		/* setup bootargs */
		ret = cmdline_merge(orig_bootargs, bootargs.ap, bootargs.used,
				    &new_cmdline);
		if (!ret) {
			panic("Error: failed to generate new kernel cmdline\n");
			return;
		}

		debug("%s: new cmdline = \"%s\"\n", __func__, new_cmdline);

		newlen = strlen(new_cmdline) + 1;

		ret = fdt_setprop(fdt, nodeoffset, "bootargs", new_cmdline,
				  newlen);
		if (ret < 0) {
			panic("Error: failed to set new kernel cmdline\n");
			return;
		}
	}

	ret = fdt_root_prop_merge(fdt);
	if (ret) {
		panic("Error: failed to set FDT root props\n");
		return;
	}

	fdt_shrink_to_minimum(fdt, 0);
}
