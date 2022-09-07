/*
 * Copyright (c) 2013-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <asm_macros.S>
#include <common/bl_common.h>
#include <mt7629_def.h>
#include <pll.h>

	.globl	plat_secondary_cold_boot_setup
	.globl	plat_report_exception
	.globl	plat_is_my_cpu_primary
	.globl  plat_my_core_pos
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush
	.globl	plat_reset_handler
	.globl	platform_mem_init

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset e.g
	 * mark the cpu's presence, mechanism to place it in a
	 * holding pen etc.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
cb_panic:
	b	cb_panic
endfunc plat_secondary_cold_boot_setup

func plat_report_exception
	bx	lr
endfunc plat_report_exception

func plat_reset_handler
	bx	lr
endfunc plat_reset_handler

func platform_mem_init
	bx	lr
endfunc platform_mem_init

func plat_is_my_cpu_primary
	ldcopr	r0, MPIDR
	ldr	r1, =(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK)
	and	r0, r1
	cmp	r0, #MT7629_PRIMARY_CPU
	moveq	r0, #1
	movne	r0, #0
	bx	lr
endfunc plat_is_my_cpu_primary

	/* -----------------------------------------------------
	 * unsigned int plat_mt_get_core_pos(void);
	 *
	 * result: CorePos = (ClusterId * 4) + CoreId
	 * -----------------------------------------------------
	 */
func plat_mt_get_core_pos
	and	r1, r0, #MPIDR_CPU_MASK
	and	r0, r0, #MPIDR_CLUSTER_MASK
	add	r0, r1, r0, LSR #6
	bx	lr
endfunc plat_mt_get_core_pos

	/* ------------------------------------
	 *  unsigned int plat_my_core_pos(void)
	 * ------------------------------------
	 */
func plat_my_core_pos
	ldcopr	r0, MPIDR
	b	plat_mt_get_core_pos
endfunc plat_my_core_pos

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0 - x4
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	r1, CLK_CFG_2
	ldr	r1, [r1]
	tst	r1, #CLK_TOP_UART_SEL_MUX_MASK
	movwne	r1, #:lower16:50000000
	movtne	r1, #:upper16:50000000
	movweq	r1, #:lower16:XTAL_CLK
	movteq	r1, #:upper16:XTAL_CLK

	mov_imm	r0, UART0_BASE
	mov_imm	r2, UART_BAUDRATE
	mov_imm	r3, 1
	b	console_hsuart_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(void)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm r1, UART0_BASE
	b	console_hsuart_core_putc
endfunc plat_crash_console_putc

	/* ---------------------------------------------
	 * int plat_crash_console_flush(int c)
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * Out : return -1 on error else return 0.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func plat_crash_console_flush
	mov_imm	r0, UART0_BASE
	b	console_hsuart_core_flush
endfunc plat_crash_console_flush