#
# Copyright (c) 2024, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
ifeq ($(FW_ENC),1)
BL31_SOURCES		+=	$(APSOC_COMMON)/img_dec/fw/fw_dec.c

BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/img_dec/fw/ \
				-DMTK_FW_ENC

ifeq ($(FW_ENC_VIA_BL31),1)
BL31_CPPFLAGS		+=	-DMTK_FW_ENC_VIA_BL31
endif

ifeq ($(FW_ENC_VIA_OPTEE),1)
BL31_CPPFLAGS		+=	-DMTK_FW_ENC_VIA_OPTEE
endif

endif # FW_ENC
