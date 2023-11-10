#
# Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

XZ_PATH	:=	lib/xz

# Imported from xz-embedded-20210201 (do not modify them)
XZ_SOURCES	:=	$(addprefix $(XZ_PATH)/,	\
					xz_dec_bcj.c	\
					xz_dec_lzma2.c	\
					xz_dec_stream.c	\
					xz_crc32.c	\
					xz_crc64.c)

# Implemented for TF
XZ_SOURCES	+=	$(addprefix $(XZ_PATH)/,	\
					tf_unxz.c)

INCLUDES	+=	-Iinclude/lib/xz

TF_CFLAGS	+=	-DXZ_DEC_SINGLE
