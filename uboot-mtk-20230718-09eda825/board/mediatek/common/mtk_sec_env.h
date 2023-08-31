/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 Mediatek Inc. All rights Reserved.
 */

#ifndef _MTK_SEC_ENV_H_
#define _MTK_SEC_ENV_H_
#include <search.h>

#define PART_U_BOOT_ENV_NAME 			"u-boot-env"

int sec_env_import(void *buf, size_t env_size);

int board_sec_env_load(void);

int get_sec_env(char *name, void **data);

#endif /* _MTK_SEC_ENV_H_ */
