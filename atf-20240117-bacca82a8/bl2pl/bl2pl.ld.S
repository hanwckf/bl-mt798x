/*
 * Copyright (c) 2021 MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <common/bl_common.ld.h>

OUTPUT_FORMAT(PLATFORM_LINKER_FORMAT)
OUTPUT_ARCH(PLATFORM_LINKER_ARCH)
ENTRY(bl2pl_entrypoint)

MEMORY {
    RAM (rwx): ORIGIN = BL2PL_BASE, LENGTH = BL2PL_LIMIT - BL2PL_BASE
}

SECTIONS
{
    . = BL2PL_BASE;

    ro . : {
        __RO_START__ = .;
        *bl2pl_entrypoint.o(.text*)
        *(SORT_BY_ALIGNMENT(.text*))
        *(SORT_BY_ALIGNMENT(.rodata*))
	RODATA_COMMON
         __RO_END__ = .;
    } >RAM

    DATA_SECTION >RAM

    . = ALIGN(4);
    .fill ALIGN(4) : {
        LONG(0xffffffff)
    } >RAM

    __COPY_END__ = .;
    __COPY_SIZE__ = __COPY_END__ - __RO_START__;

    . = ALIGN(8);
    BSS_SECTION >RAM

    ASSERT(. <= BL2PL_LIMIT, "BL2PL image has exceeded its limit.")
}
