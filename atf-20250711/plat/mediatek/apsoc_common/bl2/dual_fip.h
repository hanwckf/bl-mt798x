/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2025, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_DUAL_FIP_H_
#define _MTK_DUAL_FIP_H_

#include <stdint.h>

int dual_fip_check(const uintptr_t dev_handles[], const uintptr_t specs[],
		   uint32_t *retslot);
int dual_fip_next_slot(const uintptr_t dev_handles[], const uintptr_t specs[],
		       uint32_t *retslot);

#endif /* _MTK_DUAL_FIP_H_ */
