#
# Copyright (c) 2024, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES		+=	-I$(APSOC_COMMON)/img_dec/include

include $(APSOC_COMMON)/img_dec/roe/mtk_roe.mk

ifneq ($(filter 1,$(FW_ENC) $(FIP_ENC)),)
ifneq ($(TRUSTED_BOARD_BOOT),1)
$(error You must enable TRUSTED_BOARD_BOOT when FW_ENC or FIP_ENC enabled)
endif

CPPFLAGS		+=	-DMTK_IMG_ENC
BL_COMMON_SOURCES	+=	$(APSOC_COMMON)/img_dec/img_dec.c
BL31_SOURCES		+=	$(APSOC_COMMON)/shm.c

include $(APSOC_COMMON)/img_dec/mtk_dec_mbedtls.mk
include $(APSOC_COMMON)/img_dec/mtk_salt.mk

include $(APSOC_COMMON)/img_dec/fip/bl33_dec.mk
include $(APSOC_COMMON)/img_dec/fw/fw_dec.mk

define read_hex
    $(if $(wildcard $(2)),,$(error You must specify $(1) when FIP_ENC enabled))
    $(1):=$(shell cat $(2) | od -A n -t x2 --endian=big | sed 's/ *//g' | tr -d '\n')
endef

ifeq ($(FIP_ENC),1)
$(eval $(call read_hex,ENC_KEY,$(ENC_KEY_PATH)))
$(eval $(call read_hex,ENC_NONCE,$(ENC_NONCE_PATH)))

ENC_ARGS		+=	-d

BL2_CPPFLAGS		+=	-DMTK_FIP_ENC
BL2_SOURCES		+=	drivers/io/io_encrypted.c
endif # FIP_ENC
endif # FW_ENC or FIP_ENC
