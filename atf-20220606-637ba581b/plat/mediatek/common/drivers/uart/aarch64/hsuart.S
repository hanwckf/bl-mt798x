/*
 * SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 *
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <arch.h>
#include <asm_macros.S>
#include <assert_macros.S>
#include <console_macros.S>
#include <hsuart.h>

#define UART_MCRVAL (UART_MCR_DTR | UART_MCR_RTS)
#define UART_FCRVAL (UART_FCR_FIFO_EN | UART_FCR_RXSR | UART_FCR_TXSR)

	/*
	 * int console_hsuart_init(uintptr_t base, uint32_t clock,
	 *                         uint32_t baud, bool force_highspeed);
	 *
	 * In: x0 - base:  UART base address
	 *     w1 - clock: UART clock in Hz
	 *     w2 - baud:  Baud rate
	 *     w3 - force_highspeed: Force high-speed for low baud rates
	 * Out: return 1 on success, 0 on error
	 * Clobber list : w1, w2, w3, w4
	 */

	.globl	console_hsuart_init
func console_hsuart_init
	/* Check UART base address, clock and baud rate for sanity */
	cbz	x0, init_fail
	cbz	w1, init_fail
	cbz	w2, init_fail

	/* Disable interrupt */
	str	wzr, [x0, #UART_IER]

	/* Force DTR and RTS to high */
	mov	w4, #UART_MCRVAL
	str	w4, [x0, #UART_MCR]

	/* Clear & enable FIFOs */
	mov	w4, #UART_FCRVAL
	str	w4, [x0, #UART_FCR]

	/* Whether to use high-speed mode */
	cmp	w3, wzr
	bne	set_baud_force_high_speed

	movz	w3, #:abs_g0_nc:115200
	movk	w3, #:abs_g1:115200
	cmp	w2, w3
	ble	set_baud_low_speed

set_baud_force_high_speed:
	/* Setup high-speed baudrate mode */
	mov	w3, #3
	str	w3, [x0, #UART_HIGHSPEED]

	/*
	 * Calculate divisor:
	 * w3 = quot = DIV_ROUND_UP(clock, 256 * baud)
	 */
	sub	w3, w1, #1		/* w3 = clock - 1 */
	lsl	w4, w2, #8		/* w4 = 256 * baud */
	add	w3, w3, w4		/* w3 = clock + w4 - 1 */
	udiv	w3, w3, w4		/* w3 = (clock + 256 * baud - 1) / (256 * baud) */

	/*
	 * Calculate sample count:
	 * w1 = samplecount = DIV_ROUND_CLOSEST(clock, quot * baud)
	 */
	mul	w2, w3, w2		/* w2 = quot * baud */
	add	w1, w1, w2, lsr #1	/* w1 = clock + quot * baud / 2 */
	udiv	w1, w1, w2		/* w1 = (clock + quot * baud / 2) / (quot * baud) */

	b	set_divisor

set_baud_low_speed:
	/* Setup 16550 compatible baudrate mode */
	str	wzr, [x0, #UART_HIGHSPEED]

	/*
	 * Calculate divisor:
	 * w1 = quot = DIV_ROUND_CLOSEST(clock, 16 * baud)
	 */
	add	w1, w1, w2, lsl #3	/* w1 = clock + (baud * 16) / 2 */
	lsl	w2, w2, #4		/* w2 = baud * 16 */
	udiv	w3, w1, w2		/* w3 = (clock + 16 * baud / 2) / (16 * baud) */

	/* Set sample count to 1 */
	mov	w1, #1

set_divisor:
	/* Access to divisor latches */
	mov	w2, #(UART_LCR_WLS_8 | UART_LCR_DLAB)
	str	w2, [x0, #UART_LCR]

	/* Write 16550 divisor */
	and	w2, w3, #0xff
	str	w2, [x0, #UART_DLL]

	ubfx	w3, w3, #8, #8
	str	w3, [x0, #UART_DLM]

	/* Hide divisor latches, set 8n1 mode */
	mov	w2, #UART_LCR_WLS_8
	str	w2, [x0, #UART_LCR]

	/* Setup high-speed mode registers */
	sub	w3, w1, #1
	str	w3, [x0, #UART_SAMPLE_COUNT]

	sub	w1, w1, #2
	asr	w1, w1, #1
	str	w1, [x0, #UART_SAMPLE_POINT]

	/* Init finished */
	mov	w0, #1
	ret

init_fail:
	mov	w0, wzr
	ret
endfunc console_hsuart_init

	/*
	 * int console_hsuart_lbc_init(uintptr_t base, uint32_t clock,
	 *                             uint32_t baud);
	 *
	 * Used for UART clock <= 13MHz. High-speed mode is always enabled
	 *
	 * In: x0 - base:  UART base address
	 *     w1 - clock: UART clock in Hz
	 *     w2 - baud:  Baud rate
	 * Out: return 1 on success, 0 on error
	 * Clobber list : w1, w2, w3, w4
	 */

	.globl	console_hsuart_lbc_init
func console_hsuart_lbc_init
	/* Check UART base address, clock and baud rate for sanity */
	cbz	x0, init_fail_lbc
	cbz	w1, init_fail_lbc
	cbz	w2, init_fail_lbc

	/* Disable interrupt */
	str	wzr, [x0, #UART_IER]

	/* Force DTR and RTS to high */
	mov	w3, #UART_MCRVAL
	str	w3, [x0, #UART_MCR]

	/* Clear & enable FIFOs */
	mov	w3, #UART_FCRVAL
	str	w3, [x0, #UART_FCR]

	/* Setup high-speed baudrate mode */
	mov	w3, #3
	str	w3, [x0, #UART_HIGHSPEED]

	/* (256 * baud) is used by both branches */
	lsl	w3, w2, #8		/* w3 = 256 * baud */

	/* Choose way to calculate the divisor */
	movz	w4, #:abs_g0_nc:115200
	movk	w4, #:abs_g1:115200
	cmp	w2, w4
	bgt	set_baud_high_speed

	/*
	 * Calculate divisor for baud <= 115200:
	 * w3 = quot = DIV_ROUND_CLOSEST(clock, 256 * baud)
	 */
	add	w4, w1, w2, lsl #7	/* w4 = clock + baud * 256 / 2 */
	udiv	w3, w4, w3		/* w3 = (clock + baud * 256 / 2) / (baud * 256) */
	cmp	w3, #0
	csinc	w3, w3, wzr, ne		/* quot must be positive */
	b	set_divisor_lbc

set_baud_high_speed:
	/*
	 * Calculate divisor for baud > 115200:
	 * w3 = quot = DIV_ROUND_UP(clock, 256 * baud)
	 */
	sub	w4, w1, #1		/* w4 = clock - 1 */
	add	w4, w4, w3		/* w4 = clock + w3 - 1 */
	udiv	w3, w4, w3		/* w3 = (clock + 256 * baud - 1) / (256 * baud) */

set_divisor_lbc:
	/*
	 * Calculate sample count:
	 * w1 = samplecount = DIV_ROUND_CLOSEST(clock, quot * baud)
	 */
	mul	w2, w3, w2		/* w2 = quot * baud */
	add	w1, w1, w2, lsr #1	/* w1 = clock + quot * baud / 2 */
	udiv	w1, w1, w2		/* w1 = (clock + quot * baud / 2) / (quot * baud) */

	/* Access to divisor latches */
	mov	w2, #(UART_LCR_WLS_8 | UART_LCR_DLAB)
	str	w2, [x0, #UART_LCR]

	/* Write 16550 divisor */
	and	w2, w3, #0xff
	str	w2, [x0, #UART_DLL]

	ubfx	w3, w3, #8, #8
	str	w3, [x0, #UART_DLM]

	/* Hide divisor latches, set 8n1 mode */
	mov	w2, #UART_LCR_WLS_8
	str	w2, [x0, #UART_LCR]

	/* Setup high-speed mode registers */
	sub	w3, w1, #1
	str	w3, [x0, #UART_SAMPLE_COUNT]

	sub	w1, w1, #2
	asr	w1, w1, #1
	str	w1, [x0, #UART_SAMPLE_POINT]

	/* Init finished */
	mov	w0, #1
	ret

init_fail_lbc:
	mov	w0, wzr
	ret
endfunc console_hsuart_lbc_init

	/*
	 * int console_hsuart_core_getc(uintptr_t base);
	 *
	 * In: x0 - base:     UART base address
	 * Out: return the character from UART, or -1 if no pending char
	 * Clobber list : x0, w1
	 */

	.globl	console_hsuart_core_getc
func console_hsuart_core_getc
#if ENABLE_ASSERTIONS
	cmp	x0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	/* Check if the receive FIFO is empty */
	ldr	w1, [x0, #UART_LSR]
	tbz	w1, #0, no_char		/* UART_LSR_DR is bit 0 */
	ldr	w0, [x0, #UART_RBR]
	ret

no_char:
	mov	w0, #ERROR_NO_PENDING_CHAR
	ret
endfunc console_hsuart_core_getc

	/*
	 * int console_hsuart_core_putc(int ch, uintptr_t base);
	 *
	 * In: w0 - ch:   Character to be printed
	 * In: x1 - base: UART base address
	 * Out: return the character to be print
	 * Clobber list : w2
	 */

	.globl	console_hsuart_core_putc
func console_hsuart_core_putc
#if ENABLE_ASSERTIONS
	cmp	x1, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	/* Prepend '\r' to '\n' */
	cmp	w0, #0xA
	bne	2f
	/* Check if the transmit FIFO is full */
1:	ldr	w2, [x1, #UART_LSR]
	and	w2, w2, #(UART_LSR_TEMT | UART_LSR_THRE)
	cmp	w2, #(UART_LSR_TEMT | UART_LSR_THRE)
	bne	1b
	mov	w2, #0xD		/* '\r' */
	str	w2, [x1, #UART_THR]

	/* Check if the transmit FIFO is full */
2:	ldr	w2, [x1, #UART_LSR]
	and	w2, w2, #(UART_LSR_TEMT | UART_LSR_THRE)
	cmp	w2, #(UART_LSR_TEMT | UART_LSR_THRE)
	bne	2b
	str	w0, [x1, #UART_THR]
	ret
endfunc console_hsuart_core_putc

	/*
	 * int console_hsuart_core_flush(uintptr_t base);
	 *
	 * In: x0 - base:     UART base address
	 * Out: always 0
	 * Clobber list : x0, w1
	 */

	.globl	console_hsuart_core_flush
func console_hsuart_core_flush
#if ENABLE_ASSERTIONS
	cmp	x0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	/* Loop until the transmit FIFO is empty */
1:	ldr	w1, [x0, #UART_LSR]
	and	w1, w1, #(UART_LSR_TEMT | UART_LSR_THRE)
	cmp	w1, #(UART_LSR_TEMT | UART_LSR_THRE)
	bne	1b

	mov	w0, wzr
	ret
endfunc console_hsuart_core_flush

	/*
	 * int console_hsuart_getc(console_t *console);
	 *
	 * In: x0 - console:     pointer to console_t stucture
	 * Out: return the character from UART, or -1 if no pending char
	 * Clobber list : x0, w1
	 */
func console_hsuart_getc
#if ENABLE_ASSERTIONS
	cmp	x0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	ldr	x0, [x0, #CONSOLE_T_BASE]
	b	console_hsuart_core_getc
endfunc console_hsuart_getc

	/*
	 * int console_hsuart_putc(int ch, console_t *console);
	 *
	 * In: w0 - ch:   Character to be printed
	 * In: x1 - console:     pointer to console_t stucture
	 * Out: return the character to be print
	 * Clobber list : x1, w2
	 */

func console_hsuart_putc
#if ENABLE_ASSERTIONS
	cmp	x1, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	ldr	x1, [x1, #CONSOLE_T_BASE]
	b	console_hsuart_core_putc
endfunc console_hsuart_putc

	/*
	 * int console_hsuart_flush(console_t *console);
	 *
	 * In: x0 - console:     pointer to console_t stucture
	 * Out: always 0
	 * Clobber list : x0, w1
	 */

func console_hsuart_flush
#if ENABLE_ASSERTIONS
	cmp	x0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	ldr	x0, [x0, #CONSOLE_T_BASE]
	b	console_hsuart_core_flush
endfunc console_hsuart_flush

	/*
	 * int console_hsuart_register(uintptr_t base, uint32_t clock,
	 *                             uint32_t baud, bool force_highspeed,
	 *                             console_t *console);
	 *
	 * In: x0 - base:     UART base address
	 *     w1 - clock:    UART clock in Hz
	 *     w2 - baud:     Baud rate
	 *     w3 - force_highspeed: Force high-speed for low baud rates
	 *     x4 - console:  pointer to empty console_t struct
	 * Out: return 1 on success, 0 on error
	 * Clobber list : x0, x1, x2, x6, x7, x14
	 */

	.globl	console_hsuart_register
func console_hsuart_register
	mov	x7, x30
	mov	x6, x4
	cbz	x6, register_fail
	str	x0, [x6, #CONSOLE_T_BASE]

	/* A clock rate of zero means to skip the initialisation. */
	cbz	w1, register_hsuart

	bl	console_hsuart_init
	cbz	x0, register_fail

register_hsuart:
	mov	x0, x6
	mov	x30, x7
	finish_console_register hsuart putc=1, getc=1, flush=1

register_fail:
	ret	x7
endfunc console_hsuart_register

	/*
	 * int console_hsuart_lbc_register(uintptr_t base, uint32_t clock,
	 *                                 uint32_t baud, console_t *console);
	 *
	 * Used for UART clock <= 13MHz. High-speed mode is always enabled
	 *
	 * In: x0 - base:     UART base address
	 *     w1 - clock:    UART clock in Hz
	 *     w2 - baud:     Baud rate
	 *     x2 - console:  pointer to empty console_t struct
	 * Out: return 1 on success, 0 on error
	 * Clobber list : x0, x1, x2, x6, x7, x14
	 */

	.globl	console_hsuart_lbc_register
func console_hsuart_lbc_register
	mov	x7, x30
	mov	x6, x3
	cbz	x6, register_fail_lbc
	str	x0, [x6, #CONSOLE_T_BASE]

	/* A clock rate of zero means to skip the initialisation. */
	cbz	w1, register_hsuart_lbc

	bl	console_hsuart_lbc_init
	cbz	x0, register_fail_lbc

register_hsuart_lbc:
	mov	x0, x6
	mov	x30, x7
	finish_console_register hsuart putc=1, getc=1, flush=1

register_fail_lbc:
	ret	x7
endfunc console_hsuart_lbc_register
