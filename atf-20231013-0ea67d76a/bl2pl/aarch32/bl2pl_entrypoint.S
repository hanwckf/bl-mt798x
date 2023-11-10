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
	adr	r0, 1f
1:	mov	r1, #(1b - bl2pl_entrypoint)
	sub	r4, r0, r1

	ldr	r0, =__RO_START__
	cmp	r0, r4
	beq	3f

	/* Relocate ourself to original TEXT_BASE */
	ldr	r0, =__RO_START__
	mov	r1, r4
	ldr	r2, =__COPY_END__

2:	ldr	r3, [r1]
	str	r3, [r0]
	add	r0, r0, #4
	add	r1, r1, #4
	cmp	r0, r2
	blt	2b

	/* Clear BSS sction */
3:	ldr	r0, =__BSS_START__
	ldr	r1, =__BSS_END__
	mov	r2, #0

4:	str	r2, [r0]
	add	r0, r0, #4
	cmp	r0, r1
	blt	4b

	/* Setup stack */
	ldr	r0, =(BL2PL_LIMIT - 0x10)
	mov	sp, r0

	# Jump to real main in original TEXT_BASE
	mov	r0, r4
	ldr	r1, =bl2pl_main
	bx	r1
endfunc bl2pl_entrypoint
