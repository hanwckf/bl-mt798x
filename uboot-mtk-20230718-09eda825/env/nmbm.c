/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <command.h>
#include <env.h>
#include <env_internal.h>
#include <errno.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <malloc.h>
#include <memalign.h>
#include <search.h>

#include <nmbm/nmbm-mtd.h>

#if defined(CONFIG_CMD_SAVEENV) && defined(CONFIG_NMBM_MTD)
#define CMD_SAVEENV
#endif

#if defined(ENV_IS_EMBEDDED)
env_t *env_ptr = &environment;
#else /* ! ENV_IS_EMBEDDED */
env_t *env_ptr;
#endif /* ENV_IS_EMBEDDED */

DECLARE_GLOBAL_DATA_PTR;

static int env_nmbm_init(void)
{
#if defined(ENV_IS_EMBEDDED)
	int crc1_ok = 0, crc2_ok = 0;
	env_t *tmp_env1;

	tmp_env1 = env_ptr;
	crc1_ok = crc32(0, tmp_env1->data, ENV_SIZE) == tmp_env1->crc;

	if (!crc1_ok && !crc2_ok) {
		gd->env_addr	= 0;
		gd->env_valid	= ENV_INVALID;

		return 0;
	} else if (crc1_ok && !crc2_ok) {
		gd->env_valid = ENV_VALID;
	}

	if (gd->env_valid == ENV_VALID)
		env_ptr = tmp_env1;

	gd->env_addr = (ulong)env_ptr->data;

#else /* ENV_IS_EMBEDDED */
	gd->env_addr	= (ulong)&default_environment[0];
	gd->env_valid	= ENV_VALID;
#endif /* ENV_IS_EMBEDDED */

	return 0;
}

#ifdef CMD_SAVEENV
static int env_nmbm_save(void)
{
	ALLOC_CACHE_ALIGN_BUFFER(env_t, env_new, 1);
	struct mtd_info *mtd;
	struct erase_info ei;
	int ret = 0;

	ret = env_export(env_new);
	if (ret)
		return ret;

	mtd = nmbm_mtd_get_upper_by_index(0);
	if (!mtd)
		return 1;

	printf("Erasing on NMBM...\n");
	memset(&ei, 0, sizeof(ei));

	ei.mtd = mtd;
	ei.addr = CONFIG_ENV_OFFSET;
	ei.len = CONFIG_ENV_SIZE;

	if (mtd_erase(mtd, &ei))
		return 1;

	printf("Writing on NMBM... ");
	ret = mtd_write(mtd, CONFIG_ENV_OFFSET, CONFIG_ENV_SIZE, NULL,
			(u_char *)env_new);
	puts(ret ? "FAILED!\n" : "OK\n");

	return !!ret;
}
#endif /* CMD_SAVEENV */

static int readenv(size_t offset, u_char *buf)
{
	struct mtd_info *mtd;
	struct mtd_oob_ops ops;
	int ret;
	size_t len = CONFIG_ENV_SIZE;

	mtd = nmbm_mtd_get_upper_by_index(0);
	if (!mtd)
		return 1;

	ops.mode = MTD_OPS_AUTO_OOB;
	ops.ooblen = 0;
	while(len > 0) {
		ops.datbuf = buf;
		ops.len = min(len, (size_t)mtd->writesize);
		ops.oobbuf = NULL;

		ret = mtd_read_oob(mtd, offset, &ops);
		if (ret)
			return 1;

		buf += mtd->writesize;
		len -= mtd->writesize;
		offset += mtd->writesize;
	}

	return 0;
}

static int env_nmbm_load(void)
{
#if !defined(ENV_IS_EMBEDDED)
	ALLOC_CACHE_ALIGN_BUFFER(char, buf, CONFIG_ENV_SIZE);
	int ret;

	ret = readenv(CONFIG_ENV_OFFSET, (u_char *)buf);
	if (ret) {
		env_set_default("readenv() failed", 0);
		return -EIO;
	}

	return env_import(buf, 1, H_EXTERNAL);
#endif /* ! ENV_IS_EMBEDDED */

	return 0;
}

U_BOOT_ENV_LOCATION(nmbm) = {
	.location	= ENVL_NMBM,
	ENV_NAME("NMBM")
	.load		= env_nmbm_load,
#if defined(CMD_SAVEENV)
	.save		= env_save_ptr(env_nmbm_save),
#endif
	.init		= env_nmbm_init,
};
