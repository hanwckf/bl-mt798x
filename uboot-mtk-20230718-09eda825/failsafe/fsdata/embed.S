/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2022 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

.align	2
.section	DATA_SECT_NAME, "a"
.type	DATA_OBJ_NAME, @object
DATA_OBJ_NAME:
	.incbin DATA_OBJ_FILE
.equ	DATA_SIZE_NAME, . - DATA_OBJ_NAME
.size	DATA_OBJ_NAME, DATA_SIZE_NAME

.align	2
.section	DATA_SECT_FSPATH_NAME, "a"
.type	DATA_OBJ_FSPATH_NAME, @object
DATA_OBJ_FSPATH_NAME:
	.asciz DATA_OBJ_FSPATH
.size	DATA_OBJ_FSPATH_NAME, . - DATA_OBJ_FSPATH_NAME

.align	2
.section	UBOOT_LIST_SECT_NAME, "a"
.globl	UBOOT_LIST_NAME
.type	UBOOT_LIST_NAME, @object
UBOOT_LIST_NAME:
	.quad	DATA_OBJ_FSPATH_NAME
	.quad	DATA_SIZE_NAME
	.quad	DATA_OBJ_NAME
.size	UBOOT_LIST_NAME, . - UBOOT_LIST_NAME
