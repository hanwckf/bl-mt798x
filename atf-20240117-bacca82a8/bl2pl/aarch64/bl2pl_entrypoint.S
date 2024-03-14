/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021 MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <arch.h>
#include <asm_macros.S>
#include <common/bl_common.h>
#include <platform_def.h>

	.globl	bl2pl_entrypoint

func bl2pl_entrypoint
	/* Calculate the current load address */
	adr	x0, 1f
1:	mov	x1, #(1b - bl2pl_entrypoint)
	sub	x4, x0, x1

	ldr	x0, =__RO_START__
	cmp	x0, x4
	beq	3f

	/* Relocate ourself to original TEXT_BASE */
	ldr	x0, =__RO_START__
	mov	x1, x4
	ldr	x2, =__COPY_END__

2:	ldr	x3, [x1]
	str	x3, [x0]
	add	x0, x0, #8
	add	x1, x1, #8
	cmp	x0, x2
	blt	2b

	/* Clear BSS sction */
3:	ldr	x0, =__BSS_START__
	ldr	x1, =__BSS_END__

4:	str	xzr, [x0]
	add	x0, x0, #8
	cmp	x0, x1
	blt	4b

	/* Setup stack */
	ldr	x0, =(BL2PL_LIMIT - 0x10)
	mov	sp, x0

	/* Jump to real main in original TEXT_BASE */
	mov	x0, x4
	ldr	x1, =bl2pl_main
	br	x1
endfunc bl2pl_entrypoint
