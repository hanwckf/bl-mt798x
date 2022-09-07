/*
 * SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
 *
 * Copyright (C) 2020 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

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
	 * In: r0 - base:  UART base address
	 *     r1 - clock: UART clock in Hz
	 *     r2 - baud:  Baud rate
	 *     r3 - force_highspeed: Force high-speed for low baud rates
	 * Out: return 1 on success, 0 on error
	 * Clobber list : r1, r2, r3, ip
	 */

	.globl	console_hsuart_init
func console_hsuart_init
	/* Check UART base address, clock and baud rate for sanity */
	cmp	r0, #0
	beq	init_fail
	cmp	r1, #0
	beq	init_fail
	cmp	r2, #0
	beq	init_fail

	/* Disable interrupt */
	mov	ip, #0
	str	ip, [r0, #UART_IER]

	/* Force DTR and RTS to high */
	mov	ip, #UART_MCRVAL
	str	ip, [r0, #UART_MCR]

	/* Clear & enable FIFOs */
	mov	ip, #UART_FCRVAL
	str	ip, [r0, #UART_FCR]

	/* Whether to use high-speed mode */
	cmp	r3, #0
	bne	set_baud_force_high_speed

	movw    r3, #:lower16:115200
	movt    r3, #:upper16:115200
	cmp	r2, r3
	ble	set_baud_low_speed

set_baud_force_high_speed:
	/* Setup high-speed baudrate mode */
	mov	r3, #3
	str	r3, [r0, #UART_HIGHSPEED]

	/*
	 * Calculate divisor:
	 * r3 = quot = DIV_ROUND_UP(clock, 256 * baud)
	 */
	sub	r3, r1, #1	@ r3 = clock - 1
	lsl	ip, r2, #8	@ ip = 256 * baud
	add	r3, r3, ip	@ r3 = clock + ip - 1
	udiv	r3, r3, ip	@ r3 = (clock + 256 * baud - 1) / (256 * baud)

	/*
	 * Calculate sample count:
	 * r1 = samplecount = DIV_ROUND_CLOSEST(clock, quot * baud)
	 */
	mul	r2, r3, r2		@ r2 = quot * baud
	add	r1, r1, r2, lsr #1	@ r1 = clock + quot * baud / 2
	udiv	r1, r1, r2	@ r1 = (clock + quot * baud / 2) / (quot * baud)

	b	set_divisor

set_baud_low_speed:
	/* Setup 16550 compatible baudrate mode */
	mov	r3, #0
	str	r3, [r0, #UART_HIGHSPEED]

	/*
	 * Calculate divisor:
	 * r1 = quot = DIV_ROUND_CLOSEST(clock, 16 * baud)
	 */
	add	r1, r1, r2, lsl #3	@ r1 = clock + (baud * 16) / 2
	lsl	r2, r2, #4		@ r2 = baud * 16
	udiv	r3, r1, r2	@ r3 = (clock + 16 * baud / 2) / (16 * baud)

	/* Set sample count to 1 */
	mov	r1, #1

set_divisor:
	/* Access to divisor latches */
	mov	r2, #(UART_LCR_WLS_8 | UART_LCR_DLAB)
	str	r2, [r0, #UART_LCR]

	/* Write 16550 divisor */
	uxtb	r2, r3
	str	r2, [r0, #UART_DLL]

	ubfx	r3, r3, #8, #8
	str	r3, [r0, #UART_DLM]

	/* Hide divisor latches, set 8n1 mode */
	mov	r2, #(UART_LCR_WLS_8)
	str	r2, [r0, #UART_LCR]

	/* Setup high-speed mode registers */
	sub	r3, r1, #1
	str	r3, [r0, #UART_SAMPLE_COUNT]

	sub	r1, r1, #2
	asr	r1, r1, #1
	str	r1, [r0, #UART_SAMPLE_POINT]

	/* Init finished */
	mov	r0, #1
	bx	lr

init_fail:
	mov	r0, #0
	bx	lr
endfunc console_hsuart_init

	/*
	 * int console_hsuart_lbc_init(uintptr_t base, uint32_t clock,
	 *                             uint32_t baud);
	 *
	 * Used for UART clock <= 13MHz. High-speed mode is always enabled
	 *
	 * In: r0 - base:  UART base address
	 *     r1 - clock: UART clock in Hz
	 *     r2 - baud:  Baud rate
	 * Out: return 1 on success, 0 on error
	 * Clobber list : r1, r2, r3, ip
	 */

	.globl	console_hsuart_lbc_init
func console_hsuart_lbc_init
	/* Check UART base address, clock and baud rate for sanity */
	cmp	r0, #0
	beq	init_fail_lbc
	cmp	r1, #0
	beq	init_fail_lbc
	cmp	r2, #0
	beq	init_fail_lbc

	/* Disable interrupt */
	mov	r3, #0
	str	r3, [r0, #UART_IER]

	/* Force DTR and RTS to high */
	mov	r3, #UART_MCRVAL
	str	r3, [r0, #UART_MCR]

	/* Clear & enable FIFOs */
	mov	r3, #UART_FCRVAL
	str	r3, [r0, #UART_FCR]

	/* Setup high-speed baudrate mode */
	mov	r3, #3
	str	r3, [r0, #UART_HIGHSPEED]

	/* Choose way to calculate the divisor */
	movw    r3, #:lower16:115200
	movt    r3, #:upper16:115200
	cmp	r2, r3
	lsl	r3, r2, #8	@ r3 = 256 * baud

	/*
	 * Calculate divisor for baud > 115200:
	 * r3 = quot = DIV_ROUND_UP(clock, 256 * baud)
	 */
	subgt	ip, r1, #1	@ ip = clock - 1
	addgt	ip, ip, r3	@ ip = clock + r3 - 1
	udivgt	r3, ip, r3	@ r3 = (clock + 256 * baud - 1) / (256 * baud)
	bgt	set_divisor_lbc

	/*
	 * Calculate divisor for baud <= 115200:
	 * r3 = quot = DIV_ROUND_CLOSEST(clock, 256 * baud)
	 */
	add	ip, r1, r2, lsl #7	@ ip = clock + baud * 256 / 2
	udiv	r3, ip, r3	@ r3 = (clock + baud * 256 / 2) / (baud * 256)
	cmp	r3, #0
	moveq	r3, #1		@ quot must be positive

set_divisor_lbc:
	/*
	 * Calculate sample count:
	 * r1 = samplecount = DIV_ROUND_CLOSEST(clock, quot * baud)
	 */
	mul	r2, r3, r2		@ r2 = quot * baud
	add	r1, r1, r2, lsr #1	@ r1 = clock + quot * baud / 2
	udiv	r1, r1, r2	@ r1 = (clock + quot * baud / 2) / (quot * baud)

	/* Access to divisor latches */
	mov	r2, #(UART_LCR_WLS_8 | UART_LCR_DLAB)
	str	r2, [r0, #UART_LCR]

	/* Write 16550 divisor */
	uxtb	r2, r3
	str	r2, [r0, #UART_DLL]

	ubfx	r3, r3, #8, #8
	str	r3, [r0, #UART_DLM]

	/* Hide divisor latches, set 8n1 mode */
	mov	r2, #(UART_LCR_WLS_8)
	str	r2, [r0, #UART_LCR]

	/* Setup high-speed mode registers */
	sub	r3, r1, #1
	str	r3, [r0, #UART_SAMPLE_COUNT]

	sub	r1, r1, #2
	asr	r1, r1, #1
	str	r1, [r0, #UART_SAMPLE_POINT]

	/* Init finished */
	mov	r0, #1
	bx	lr

init_fail_lbc:
	mov	r0, #0
	bx	lr
endfunc console_hsuart_lbc_init

	/*
	 * int console_hsuart_core_getc(uintptr_t base);
	 *
	 * In: r0 - base:     UART base address
	 * Out: return the character from UART, or -1 if no pending char
	 * Clobber list : r0, r1
	 */

	.globl	console_hsuart_core_getc
func console_hsuart_core_getc
#if ENABLE_ASSERTIONS
	cmp	r0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	/* Check if the receive FIFO is empty */
	ldr	r1, [r0, #UART_LSR]
	tst	r1, #UART_LSR_DR
	beq	no_char
	ldr	r1, [r0, #UART_RBR]
	mov	r0, r1
	bx	lr

no_char:
	mov	r0, #ERROR_NO_PENDING_CHAR
	bx	lr
endfunc console_hsuart_core_getc

	/*
	 * int console_hsuart_core_putc(int ch, uintptr_t base);
	 *
	 * In: r0 - ch:   Character to be printed
	 * In: r1 - base: UART base address
	 * Out: return the character to be print
	 * Clobber list : r2
	 */

	.globl	console_hsuart_core_putc
func console_hsuart_core_putc
#if ENABLE_ASSERTIONS
	cmp	r1, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	/* Prepend '\r' to '\n' */
	cmp	r0, #0xA
	bne	2f
	/* Check if the transmit FIFO is full */
1:	ldr	r2, [r1, #UART_LSR]
	and	r2, r2, #(UART_LSR_TEMT | UART_LSR_THRE)
	cmp	r2, #(UART_LSR_TEMT | UART_LSR_THRE)
	bne	1b
	mov	r2, #0xD		/* '\r' */
	str	r2, [r1, #UART_THR]

	/* Check if the transmit FIFO is full */
2:	ldr	r2, [r1, #UART_LSR]
	and	r2, r2, #(UART_LSR_TEMT | UART_LSR_THRE)
	cmp	r2, #(UART_LSR_TEMT | UART_LSR_THRE)
	bne	2b
	str	r0, [r1, #UART_THR]
	bx	lr
endfunc console_hsuart_core_putc

	/*
	 * int console_hsuart_core_flush(uintptr_t base);
	 *
	 * In: r0 - base:     UART base address
	 * Out: always 0
	 * Clobber list : r0, r1
	 */

	.globl	console_hsuart_core_flush
func console_hsuart_core_flush
#if ENABLE_ASSERTIONS
	cmp	r0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	/* Loop until the transmit FIFO is empty */
1:	ldr	r1, [r0, #UART_LSR]
	and	r1, r1, #(UART_LSR_TEMT | UART_LSR_THRE)
	cmp	r1, #(UART_LSR_TEMT | UART_LSR_THRE)
	bne	1b

	mov	r0, #0
	bx	lr
endfunc console_hsuart_core_flush

	/*
	 * int console_hsuart_getc(console_t *console);
	 *
	 * In: r0 - console:     pointer to console_t stucture
	 * Out: return the character from UART, or -1 if no pending char
	 * Clobber list : r0, r1
	 */
func console_hsuart_getc
#if ENABLE_ASSERTIONS
	cmp	r0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	ldr	r0, [r0, #CONSOLE_T_BASE]
	b	console_hsuart_core_getc
endfunc console_hsuart_getc

	/*
	 * int console_hsuart_putc(int ch, console_t *console);
	 *
	 * In: r0 - ch:   Character to be printed
	 * In: r1 - console:     pointer to console_t stucture
	 * Out: return the character to be print
	 * Clobber list : r0, r1
	 */

func console_hsuart_putc
#if ENABLE_ASSERTIONS
	cmp	r1, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	ldr	r1, [r1, #CONSOLE_T_BASE]
	b	console_hsuart_core_putc
endfunc console_hsuart_putc

	/*
	 * int console_hsuart_flush(console_t *console);
	 *
	 * In: r0 - console:     pointer to console_t stucture
	 * Out: always 0
	 * Clobber list : r0, r1
	 */

func console_hsuart_flush
#if ENABLE_ASSERTIONS
	cmp	r0, #0
	ASM_ASSERT(ne)
#endif /* ENABLE_ASSERTIONS */

	ldr	r0, [r0, #CONSOLE_T_BASE]
	b	console_hsuart_core_flush
endfunc console_hsuart_flush

	/*
	 * int console_hsuart_register(uintptr_t base, uint32_t clock,
	 *                             uint32_t baud, bool force_highspeed,
	 *                             console_t *console);
	 *
	 * In: r0 - base:     UART base address
	 *     r1 - clock:    UART clock in Hz
	 *     r2 - baud:     Baud rate
	 *     r3 - force_highspeed: Force high-speed for low baud rates
	 *     sp[0] - console:  pointer to empty console_t struct
	 * Out: return 1 on success, 0 on error
	 * Clobber list : r1, r2, r3, ip
	 */

	.globl	console_hsuart_register
func console_hsuart_register
	push	{r4, lr}
	ldr	r4, [sp, #8]
	cmp	r4, #0
	beq	register_fail
	str	r0, [r4, #CONSOLE_T_BASE]

	/* A clock rate of zero means to skip the initialisation. */
	cmp	r1, #0
	beq	register_hsuart

	bl	console_hsuart_init
	cmp	r0, #0
	beq	register_fail

register_hsuart:
	mov	r0, r4
	pop	{r4, lr}
	finish_console_register hsuart putc=1, getc=1, flush=1

register_fail:
	pop	{r4, lr}

	cmp	r0, #0
	bx	lr
endfunc console_hsuart_register

	/*
	 * int console_hsuart_lbc_register(uintptr_t base, uint32_t clock,
	 *                                 uint32_t baud, console_t *console);
	 *
	 * Used for UART clock <= 13MHz. High-speed mode is always enabled
	 *
	 * In: r0 - base:     UART base address
	 *     r1 - clock:    UART clock in Hz
	 *     r2 - baud:     Baud rate
	 *     r2 - console:  pointer to empty console_t struct
	 * Out: return 1 on success, 0 on error
	 * Clobber list : r1, r2, r3, ip
	 */

	.globl	console_hsuart_lbc_register
func console_hsuart_lbc_register
	push	{r4, lr}
	mov	r4, r3
	cmp	r4, #0
	beq	register_fail_lbc
	str	r0, [r4, #CONSOLE_T_BASE]

	/* A clock rate of zero means to skip the initialisation. */
	cmp	r1, #0
	beq	register_hsuart_lbc

	bl	console_hsuart_lbc_init
	cmp	r0, #0
	beq	register_fail_lbc

register_hsuart_lbc:
	mov	r0, r4
	pop	{r4, lr}
	finish_console_register hsuart putc=1, getc=1, flush=1

register_fail_lbc:
	pop	{r4, lr}

	cmp	r0, #0
	bx	lr
endfunc console_hsuart_lbc_register
