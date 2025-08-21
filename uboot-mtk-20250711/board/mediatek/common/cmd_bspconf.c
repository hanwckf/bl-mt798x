// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * BSP configuration command
 */

#include <stdio.h>
#include <string.h>
#include <command.h>
#include <vsprintf.h>
#include "bsp_conf.h"

enum bspconf_field_type {
	FIELD_U32,
};

enum bspconf_field_format {
	FORMAT_NORMAL,
	FORMAT_HEX,
	FORMAT_HEX_FULL,
};

struct bspconf_field_info {
	const char *key;
	enum bspconf_field_type type;
	enum bspconf_field_format format;
	u32 offset;
	u32 len;
	u64 min_val;
	u64 max_val;
};

#define BSPCONF_FIELD_U32(_field, _min, _max)	\
	{ .key = (#_field), .type = FIELD_U32, .len = sizeof(u32), \
	  .offset = offsetof(struct mtk_bsp_conf_data, _field), \
	  .min_val = (_min), .max_val = (_max) }

#define BSPCONF_FIELD_U32_FMT(_field, _min, _max, _format)	\
	{ .key = (#_field), .type = FIELD_U32, .len = sizeof(u32), \
	  .offset = offsetof(struct mtk_bsp_conf_data, _field), \
	  .min_val = (_min), .max_val = (_max), .format = (_format) }

static const struct bspconf_field_info bspconf_fields[] = {
	BSPCONF_FIELD_U32(current_fip_slot, 0, FIP_NUM - 1),
	BSPCONF_FIELD_U32(fip[0].invalid, 0, UINT32_MAX),
	BSPCONF_FIELD_U32(fip[0].size, 0, UINT32_MAX),
	BSPCONF_FIELD_U32_FMT(fip[0].crc32, 0, UINT32_MAX, FORMAT_HEX_FULL),
	BSPCONF_FIELD_U32(fip[0].upd_cnt, 0, UINT32_MAX),
	BSPCONF_FIELD_U32(fip[1].invalid, 0, UINT32_MAX),
	BSPCONF_FIELD_U32(fip[1].size, 0, UINT32_MAX),
	BSPCONF_FIELD_U32_FMT(fip[1].crc32, 0, UINT32_MAX, FORMAT_HEX_FULL),
	BSPCONF_FIELD_U32(fip[1].upd_cnt, 0, UINT32_MAX),
#if 0
	BSPCONF_FIELD_U32(current_image_slot, 0, IMAGE_NUM - 1),
	BSPCONF_FIELD_U32(image[0].invalid, 0, UINT32_MAX),
	BSPCONF_FIELD_U32(image[0].size, 0, UINT32_MAX),
	BSPCONF_FIELD_U32_FMT(image[0].crc32, 0, UINT32_MAX, FORMAT_HEX_FULL),
	BSPCONF_FIELD_U32(image[0].upd_cnt, 0, UINT32_MAX),
	BSPCONF_FIELD_U32(image[1].invalid, 0, UINT32_MAX),
	BSPCONF_FIELD_U32(image[1].size, 0, UINT32_MAX),
	BSPCONF_FIELD_U32_FMT(image[1].crc32, 0, UINT32_MAX, FORMAT_HEX_FULL),
	BSPCONF_FIELD_U32(image[1].upd_cnt, 0, UINT32_MAX),
#endif
};

static int do_bspconf_help(int exitcode)
{
	printf("Usage:\n");
	printf("    bspconf help\n");
	printf("    bspconf show\n");
	printf("    bspconf save\n");
	printf("    bspconf set <key> <value>\n");

	return exitcode;
}

static int do_bspconf_show(void)
{;
	const u32 *pu32;
	u32 i;

	printf("Current configuration index: %u\n", curr_bspconf_index);
	printf("Current configuration update count: %u\n", curr_bspconf.upd_cnt);
	printf("\n");

	for (i = 0; i < ARRAY_SIZE(bspconf_fields); i++) {
		switch (bspconf_fields[i].type) {
		case FIELD_U32:
			pu32 = (u32 *)((void *)&curr_bspconf + bspconf_fields[i].offset);

			switch (bspconf_fields[i].format) {
			case FORMAT_NORMAL:
				printf("%s = %u\n", bspconf_fields[i].key,
				       *pu32);
				break;

			case FORMAT_HEX:
				printf("%s = 0x%x\n", bspconf_fields[i].key,
				       *pu32);
				break;

			case FORMAT_HEX_FULL:
				printf("%s = %08x\n", bspconf_fields[i].key,
				       *pu32);
				break;
			}

			break;
		}
	}

	return 0;
}

static int do_bspconf_save(int argc, char *const argv[])
{
	return save_bsp_conf();
}

static int do_bspconf_set(int argc, char *const argv[])
{
	const struct bspconf_field_info *field = NULL;
	const char *key, *val;
	u32 i, vu32, *pu32;
	char *end;

	if (argc < 3) {
		if (argc == 1)
			printf("Key and value not specified\n");
		else
			printf("Value not specified\n");

		return 1;
	}

	key = argv[1];
	val = argv[2];

	for (i = 0; i < ARRAY_SIZE(bspconf_fields); i++) {
		if (!strcmp(key, bspconf_fields[i].key)) {
			field = &bspconf_fields[i];
			break;
		}
	}

	if (!field) {
		printf("Key '%s' is not valid\n", key);
		return 1;
	}

	switch (field->type) {
	case FIELD_U32:
		pu32 = (u32 *)((void *)&curr_bspconf + field->offset);

		vu32 = simple_strtoul(val, &end, 0);
		if (end == val || *end) {
			printf("Value is not valid\n");
			return 1;
		}

		if (vu32 < field->min_val || vu32 > field->max_val) {
			printf("Value %u is not in valid range\n", vu32);
			return 1;
		}

		*pu32 = vu32;
		break;
	}

	return 0;
}

static int do_bspconf(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	if (argc == 1)
		return do_bspconf_show();

	if (!strcmp(argv[1], "help"))
		return do_bspconf_help(0);

	if (!strcmp(argv[1], "show"))
		return do_bspconf_show();

	if (!strcmp(argv[1], "save"))
		return do_bspconf_save(argc - 1, argv + 1);

	if (!strcmp(argv[1], "set"))
		return do_bspconf_set(argc - 1, argv + 1);

	return do_bspconf_help(1);
}

U_BOOT_CMD(bspconf, CONFIG_SYS_MAXARGS, 0, do_bspconf, "MTK BSP configuration",
	   NULL);
