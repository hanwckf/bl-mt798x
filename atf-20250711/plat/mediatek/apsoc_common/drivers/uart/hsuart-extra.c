// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
/*
 * Copyright (C) 2025 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <lib/mmio.h>
#include "hsuart.h"

#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(x) __x = x;				\
	typeof(divisor) __d = divisor;			\
	(((__x) + ((__d) / 2)) / (__d));		\
}							\
)

#define DIV_ROUND_UP(x, divisor)(			\
{							\
	typeof(x) __x = x;				\
	typeof(divisor) __d = divisor;			\
	(((__x) + (__d) - 1) / (__d));			\
}							\
)

static const unsigned short fraction_l_mapping[] = {
	0, 1, 0x5, 0x15, 0x55, 0x57, 0x57, 0x77, 0x7F, 0xFF, 0xFF
};

static const unsigned short fraction_m_mapping[] = {
	0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3
};

void hsuart_set_baud(uintptr_t base, uint32_t clock, uint32_t baud,
		     bool force_highspeed)
{
	uint32_t quot, sample_count = 1, sample_point, dll, dlm, fraction;
	uint32_t highspeed = 0, frac_l = 0, frac_m = 0;

	if (force_highspeed) {
		quot = DIV_ROUND_UP(clock, 256 * baud);
		sample_count = clock / (quot * baud);
		highspeed = 3;

		fraction = ((clock * 100) / (quot * baud)) % 100;
		fraction = DIV_ROUND_CLOSEST(fraction, 10);

		frac_l = fraction_l_mapping[fraction];
		frac_m = fraction_m_mapping[fraction];
	} else {
		quot = DIV_ROUND_CLOSEST(clock, 16 * baud);
	}

	sample_point = sample_count / 2;
	dll = quot & 0xff;
	dlm = (quot >> 8) & 0xff;

	mmio_setbits_32(base + UART_LCR, UART_LCR_DLAB);
	mmio_write_32(base + UART_DLM, dlm);
	mmio_write_32(base + UART_DLL, dll);
	mmio_clrbits_32(base + UART_LCR, UART_LCR_DLAB);

	mmio_write_32(base + UART_HIGHSPEED, highspeed);
	mmio_write_32(base + UART_SAMPLE_COUNT, sample_count - 1);
	mmio_write_32(base + UART_SAMPLE_POINT, sample_point - 1);

	mmio_write_32(base + UART_FRACDIV_L, frac_l);
	mmio_write_32(base + UART_FRACDIV_M, frac_m);
}

bool hsuart_pending(uintptr_t base, bool input)
{
	if (input)
		return !!(mmio_read_32(base + UART_LSR) & UART_LSR_DR);

	return (mmio_read_32(base + UART_LSR) & UART_LSR_THRE) ? false : true;
}

bool hsuart_tx_done(uintptr_t base)
{
	uint32_t mask = UART_LSR_THRE | UART_LSR_TEMT;

	return (mmio_read_32(base + UART_LSR) & mask) == mask;
}

int __hsuart_getc(uintptr_t base)
{
	return mmio_read_32(base + UART_RBR);
}

void __hsuart_putc(uintptr_t base, int val)
{
	mmio_write_32(base + UART_THR, val);
}

int hsuart_getc(uintptr_t base)
{
	while (!hsuart_pending(base, true))
		;

	return mmio_read_32(base + UART_RBR);
}

void hsuart_putc(uintptr_t base, int val)
{
	while (hsuart_pending(base, false))
		;

	mmio_write_32(base + UART_THR, val);
}
