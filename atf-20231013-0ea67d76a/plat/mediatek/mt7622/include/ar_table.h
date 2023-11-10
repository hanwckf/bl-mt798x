/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef AR_TABLE_H
#define AR_TABLE_H

#include <stdint.h>
#include <mt7622_def.h>

#define AR_ENTRY(_idx, _bl2, _fip, _fit) {	\
                .release = _idx,		\
                .bl2_ar_ver = _bl2,		\
                .tfw_ar_ver = _fip,		\
                .ntfw_ar_ver = _fip,		\
                .fit_ar_ver = _fit		\
        }

struct ar_table_entry {
	uint32_t release;
	uint32_t bl2_ar_ver;
	uint32_t tfw_ar_ver;
	uint32_t ntfw_ar_ver;
	uint32_t fit_ar_ver;
};

int mtk_antirollback_get_bl2_ar_ver(void);
int mtk_antirollback_get_tfw_ar_ver(void);
int mtk_antirollback_get_ntfw_ar_ver(void);
int mtk_antirollback_get_fit_ar_ver(uint32_t *fit_ar_ver);
int mtk_antirollback_update_efuse_ar_ver(void);

#endif /* AR_TABLE_H */
