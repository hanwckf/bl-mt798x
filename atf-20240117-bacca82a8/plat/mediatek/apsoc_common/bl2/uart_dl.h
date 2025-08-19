// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#ifndef _MTK_UART_DL_H_
#define _MTK_UART_DL_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <drivers/console.h>

void start_uart_dl(uintptr_t loadaddr);

extern int uart_dl_api_getc(void);
extern void uart_dl_api_putc(int ch);
extern void uart_dl_api_set_baud(uint32_t baudrate);

#endif /* _MTK_UART_DL_H_ */
