
// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (C) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */
#include <stdbool.h>
#include <lib/mmio.h>
#include <mt7981_def.h>
#include <hsuart.h>

#define UART_TX_IDLE		(UART_LSR_TEMT | UART_LSR_THRE)

void xz_simple_putc(int ch)
{
	uint32_t val;

	mmio_write_32(UART_BASE + UART_THR, ch);

	do {
		val = mmio_read_32(UART_BASE + UART_LSR);
	} while ((val & UART_TX_IDLE) != UART_TX_IDLE);
}
