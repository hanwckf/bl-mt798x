// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic image boot helper
 */

#include <errno.h>
#include <image.h>
#include <malloc.h>
#include <linux/types.h>
#include <linux/sizes.h>
#include <linux/ctype.h>
#include <linux/string.h>

#include "upgrade_helper.h"
#include "boot_helper.h"
#include "dm_parser.h"

int boot_from_mem(ulong data_load_addr)
{
	char cmd[64];

	snprintf(cmd, sizeof(cmd), "bootm 0x%lx", data_load_addr);

	return run_command(cmd, 0);
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

static bool bootargs_find(const struct bootarg *bootargs, u32 count,
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

int cmdline_merge(const char *cmdline, const struct bootarg *bootargs,
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
	if (!dm_mod) {
		panic("Error: dm-mod.create not found in original bootargs!\n");
		return -EINVAL;
	}
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
	}
#endif

	if (b > buff)
		b[-1] = 0;

	*result = buff;
	return 1;
}
