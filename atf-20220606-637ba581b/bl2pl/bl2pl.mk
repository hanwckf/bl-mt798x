#
# Copyright (c) 2021 MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xz/xz.mk

BL2PL_SOURCES		+=	bl2pl/bl2pl_main.c			\
				bl2pl/${ARCH}/bl2pl_entrypoint.S	\
				$(XZ_SOURCES)

BL2PL_LINKERFILE	:=	bl2pl/bl2pl.ld.S
