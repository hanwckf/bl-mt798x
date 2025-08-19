/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PINCTRL_H_
#define PINCTRL_H_

void mtk_set_pin_mode(int gpio, int mode);
int mtk_get_pin_mode(int gpio);
void mtk_pin_init(void);

#endif
