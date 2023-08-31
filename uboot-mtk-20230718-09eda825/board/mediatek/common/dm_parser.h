/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * dm-verity command line parser
 */

#ifndef _DM_PARSER_H_
#define _DM_PARSER_H_

#include <linux/types.h>

struct dm_info {
	char *tmpbuf;
};

struct dm_verity_info {
	char *tmpbuf;

	char *datadev;
	uint32_t datadev_pos;
	uint32_t datadev_len;

	char *hashdev;
	uint32_t hashdev_pos;
	uint32_t hashdev_len;

	uint32_t datadev_blksz;
	uint32_t hashdev_blksz;
	uint64_t data_blocks;
	uint64_t hash_startblock;

	char *digest_name;
};

struct dm_crypt_info {
	char *tmpbuf;

	char *key;
	uint32_t key_pos;
	uint32_t key_len;
};

int dm_mod_create_arg_parse(const char *str, char *target_type, struct dm_info *di);

#endif /* _DM_PARSER_H_ */
