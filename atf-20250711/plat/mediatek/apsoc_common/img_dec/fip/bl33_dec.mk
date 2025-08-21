#
# Copyright (c) 2024, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(ENCRYPT_BL33),1)
BL31_SOURCES		+=	$(APSOC_COMMON)/img_dec/fip/bl33_dec.c

BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/img_dec/fip \
				-DMTK_FIP_ENC

endif
