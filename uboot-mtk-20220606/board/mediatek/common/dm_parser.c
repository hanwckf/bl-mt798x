/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * dm-verity command line parser
 */

#include <malloc.h>
#include <vsprintf.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include "dm_parser.h"

/**
 * str_field_delimit - delimit a string based on a separator char.
 * @str: the pointer to the string to delimit.
 * @separator: char that delimits the field
 *
 * Find a @separator and replace it by '\0'.
 * Remove leading and trailing spaces.
 * Return the remainder string after the @separator.
 */
static char *str_field_delimit(char **str, char separator)
{
	char *s;

	/* TODO: add support for escaped characters */
	*str = skip_spaces(*str);
	s = strchr(*str, separator);
	/* Delimit the field and remove trailing spaces */
	if (s)
		*s = '\0';
	*str = strim(*str);
	return s ? ++s : NULL;
}

/*
 * Destructively splits up the argument list to pass to ctr.
 */
static int dm_split_args(int max_argv, char *argv[], char *input)
{
	char *start, *end = input, *out;
	int argc = 0;

	if (!input)
		return 0;

	while (1) {
		/* Skip whitespace */
		start = skip_spaces(end);

		if (!*start)
			break;	/* success, we hit the end */

		/* 'out' is used to remove any back-quotes */
		end = out = start;
		while (*end) {
			/* Everything apart from '\0' can be quoted */
			if (*end == '\\' && *(end + 1)) {
				*out++ = *(end + 1);
				end += 2;
				continue;
			}

			if (isspace(*end))
				break;	/* end of token */

			*out++ = *end++;
		}

		/* we know this is whitespace */
		if (*end)
			end++;

		/* terminate the string and put it in the array */
		*out = '\0';
		argv[argc] = start;
		argc++;

		/* have we already filled the array ? */
		if (argc >= max_argv)
			break;
	}

	return argc;
}

static int dm_parse_params(char *str, bool require_hash_blocks, struct dm_verity_info *dvi)
{
	char *argv[10];
	int argc;

	argc = dm_split_args(ARRAY_SIZE(argv), argv, str);

	if (argc < 3)
		return -EINVAL;

	dvi->datadev = argv[1];
	dvi->datadev_pos = dvi->datadev - dvi->tmpbuf;
	dvi->datadev_len = strlen(dvi->datadev);

	dvi->hashdev = argv[2];
	dvi->hashdev_pos = dvi->hashdev - dvi->tmpbuf;
	dvi->hashdev_len = strlen(dvi->hashdev);

	if (require_hash_blocks && argc < 8)
		return -EINVAL;

	dvi->datadev_blksz = simple_strtoul(argv[3], NULL, 0);
	dvi->hashdev_blksz = simple_strtoul(argv[4], NULL, 0);
	dvi->data_blocks = simple_strtoull(argv[5], NULL, 0);
	dvi->hash_startblock = simple_strtoull(argv[6], NULL, 0);

	dvi->digest_name = argv[7];

	return 0;
}

/**
 * dm_parse_table_entry - parse a table entry
 * @str: the pointer to a string with the format:
 *	<start_sector> <num_sectors> <target_type> <target_args>[, ...]
 *
 * Return the remainder string after the table entry, i.e, after the comma which
 * delimits the entry or NULL if reached the end of the string.
 */
static int dm_parse_table_entry(char *str, bool require_hash_blocks, struct dm_verity_info *dvi)
{
	unsigned int i;
	/* fields:  */
	char *field[4];

	field[0] = str;
	/* Delimit first 3 fields that are separated by space */
	for (i = 0; i < ARRAY_SIZE(field) - 1; i++) {
		field[i + 1] = str_field_delimit(&field[i], ' ');
		if (!field[i + 1])
			return -EINVAL;
	}

	/* params */
	return dm_parse_params(field[3], require_hash_blocks, dvi);
}

/**
 * dm_parse_device_entry - parse a device entry
 * @str: the pointer to a string with the format:
 *	name,uuid,minor,flags,table[; ...]
 *
 * Return the remainder string after the table entry, i.e, after the semi-colon
 * which delimits the entry or NULL if reached the end of the string.
 */
static int dm_parse_device_entry(char *str, bool require_hash_blocks, struct dm_verity_info *dvi)
{
	/* There are 5 fields: name,uuid,minor,flags,table; */
	char *field[5];
	unsigned int i;

	field[0] = str;
	/* Delimit first 4 fields that are separated by comma */
	for (i = 0; i < ARRAY_SIZE(field) - 1; i++) {
		field[i + 1] = str_field_delimit(&field[i], ',');
		if (!field[i + 1])
			return -EINVAL;
	}

	/* table */
	return dm_parse_table_entry(field[4], require_hash_blocks, dvi);
}

int dm_mod_create_arg_parse(const char *str, struct dm_verity_info *dvi)
{
	char *dmstr = strdup(str);
	int ret;

	if (!dmstr)
		return -ENOMEM;

	dvi->tmpbuf = dmstr;

	ret = dm_parse_device_entry(dmstr, false, dvi);

	return ret;
}
