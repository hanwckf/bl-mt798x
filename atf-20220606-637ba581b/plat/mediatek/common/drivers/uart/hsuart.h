// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#ifndef _MTK_HSUART_H_
#define _MTK_HSUART_H_

/* UART register */
#define UART_RBR		0x00	/* Receive buffer register */
#define UART_DLL		0x00	/* Divisor latch lsb */
#define UART_THR		0x00	/* Transmit holding register */
#define UART_DLM		0x04	/* Divisor latch msb */
#define UART_IER		0x04	/* Interrupt enable register */
#define UART_FCR		0x08	/* FIFO control register */
#define UART_LCR		0x0c	/* Line control register */
#define UART_MCR		0x10	/* Modem control register */
#define UART_LSR		0x14	/* Line status register */
#define UART_HIGHSPEED		0x24	/* High speed UART */
#define UART_SAMPLE_COUNT	0x28	/* High speed sampling count */
#define UART_SAMPLE_POINT	0x2c	/* High speed sample point */

/* FCR */
#define UART_FCR_FIFO_EN	0x01	/* enable FIFO */
#define UART_FCR_RXSR		0x02	/* clear the RCVR FIFO */
#define UART_FCR_TXSR		0x04	/* clear the XMIT FIFO */

/* LCR */
#define UART_LCR_WLS_8		0x03	/* 8 bit character length */
#define UART_LCR_DLAB		0x80	/* divisor latch access bit */

/* MCR */
#define UART_MCR_DTR		0x01
#define UART_MCR_RTS		0x02

/* LSR */
#define UART_LSR_DR		0x01	/* Data ready */
#define UART_LSR_THRE		0x20	/* Xmit holding register empty */
#define UART_LSR_TEMT		0x40	/* Xmit holding register empty */

/* UART functions */
#ifndef __ASSEMBLER__
#include <drivers/console.h>

int console_hsuart_init(uintptr_t base, uint32_t clock, uint32_t baud,
			bool force_highspeed);
int console_hsuart_lbc_init(uintptr_t base, uint32_t clock, uint32_t baud);
int console_hsuart_core_getc(uintptr_t base);
int console_hsuart_core_putc(int ch, uintptr_t base);
int console_hsuart_core_flush(uintptr_t base);

int console_hsuart_register(uintptr_t base, uint32_t clock,
			    uint32_t baud, bool force_highspeed,
			    console_t *console);
int console_hsuart_lbc_register(uintptr_t base, uint32_t clock,
				uint32_t baud, console_t *console);
#endif

#endif /* _MTK_HSUART_H_ */
