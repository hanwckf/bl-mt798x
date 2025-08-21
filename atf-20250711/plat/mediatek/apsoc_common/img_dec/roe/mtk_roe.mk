#
# Copyright (c) 2024, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

BL2_CPPFLAGS		+= -DMTK_PLAT_KEY

PLAT_INCLUDES		+= -I$(APSOC_COMMON)/img_dec/roe
PREBUILT_LIBS		+= $(APSOC_COMMON)/img_dec/roe/release/mtk_plat_key.o

ifneq ($(filter 1,$(FW_ENC) $(FIP_ENC)),)
BL_COMMON_SOURCES	+= $(APSOC_COMMON)/img_dec/roe/mtk_roe.c
endif
