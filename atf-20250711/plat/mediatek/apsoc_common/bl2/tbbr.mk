#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Trusted board boot
#
ifeq ($(TRUSTED_BOARD_BOOT),1)

include drivers/auth/mbedtls/mbedtls_crypto.mk
include drivers/auth/mbedtls/mbedtls_x509.mk
include drivers/auth/auth.mk

ifeq ($(MBEDTLS_DIR),)
$(error You must specify MBEDTLS_DIR when TRUSTED_BOARD_BOOT enabled)
endif

AUTH_SOURCES		+=	drivers/auth/tbbr/tbbr_cot_bl2.c		\
				drivers/auth/tbbr/tbbr_cot_common.c

BL2_SOURCES		+=	$(AUTH_SOURCES)					\
				$(APSOC_COMMON)/bl2/mtk_tbbr.c			\
				$(APSOC_COMMON)/bl2/mtk_rotpk.S

PLAT_BL_COMMON_SOURCES	+=	$(APSOC_COMMON)/mbedtls_helper.c

ROTPK_HASH		:=	$(BUILD_PLAT)/rotpk_$(HASH_ALG).bin
$(eval $(call add_define_val,ROTPK_HASH,'"$(ROTPK_HASH)"'))
endif
