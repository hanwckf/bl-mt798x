/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _BL2PL_TIMER_H_
#define _BL2PL_TIMER_H_

#include <stdint.h>

void mtk_bl2pl_timer_init(void);
void udelay(uint32_t usec);

#endif /* _BL2PL_TIMER_H_ */
