/*
 * SPDX-License-Identifier: (GPL-2.0+ OR BSD-3-Clause)
 *
 * Copyright (C) 2021, MediaTek Inc. All rights reserved.
 */

	.align	2
	.section	".data.dtb_data", "a"
	.type	dtb_data, %object
	.global	dtb_data
dtb_data:
	.incbin	DTB_PATH
	.equ	_dtb_data_size, . - dtb_data
	.size	dtb_data, _dtb_data_size

	.align	2
	.section	".data.dtb_data_size", "a"
	.type	dtb_data_size, %object
	.global	dtb_data_size
dtb_data_size:
	.word	_dtb_data_size
	.size	dtb_data_size, . - dtb_data_size
