// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2022 Mediatek Incorporation. All Rights Reserved.
 */
#include <common.h>
#include <env_internal.h>
#include <env_flags.h>
#include <u-boot/crc.h>
#include <errno.h>
#include <search.h>
#include "mtk_sec_env.h"

static struct hsearch_data sec_env_htab = {
	.change_ok = env_flags_validate,
};

int sec_env_import(void *buf, size_t env_size)
{
	int ret;
	int crc0 = 0, crc0_ok = 0;
	env_t *env_ptr;

	if (!buf || !env_size)
		return -EINVAL;

	env_ptr = (env_t *)buf;

	crc0 = crc32(0, (uint8_t *)env_ptr->data, env_size);
	crc0_ok = (crc0 == env_ptr->crc);
	if (!crc0_ok) {
		printf("Bad CRC\n");
		return -1;
	}

	ret = himport_r(&sec_env_htab, env_ptr->data, env_size, '\0',
			H_EXTERNAL, 0, 0, NULL);

	return !ret;
}

int get_sec_env(char *name, void **data)
{
	struct env_entry e = {0}, *ep = NULL;

	if (!name || !data || !sec_env_htab.table || !sec_env_htab.size)
		return -EINVAL;

	*data = NULL;

	e.key = name;
	e.data = NULL;

	hsearch_r(e, ENV_FIND, &ep, &sec_env_htab, 0);
	if (!ep)
		return -ENOENT;

	*data = ep->data;

	return 0;
}
