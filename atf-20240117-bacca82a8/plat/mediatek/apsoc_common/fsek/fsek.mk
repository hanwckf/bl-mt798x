#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# FSEK related build macros
#
ifeq ($(FSEK),1)
ifneq ($(TRUSTED_BOARD_BOOT),1)
$(error You must enable TRUSTED_BOARD_BOOT when FSEK enabled)
endif
BL31_SOURCES		+=	drivers/auth/mbedtls/mbedtls_common.c		\
				$(APSOC_COMMON)/fsek/mtk_fsek.c
BL31_CPPFLAGS		+=	-DMTK_FSEK

#
# SW HUK related build macros
#
BL31_SOURCES		+=	$(APSOC_COMMON)/fsek/mtk_huk.c			\
				$(APSOC_COMMON)/drivers/crypto/mtk_crypto.c
BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/drivers/crypto		\
				-I$(APSOC_COMMON)/drivers/crypto/include	\
				-I$(APSOC_COMMON)/fsek
LDLIBS			+=	$(MTK_PLAT_SOC)/drivers/crypto/release/libcrypto.a

#
# SW ROEK related build macros
#
BL31_SOURCES		+=	$(APSOC_COMMON)/fsek/mtk_roek.c
ifeq ($(ENC_ROEK),1)
BL31_CPPFLAGS		+=	-DMTK_ENC_ROEK
endif
endif
