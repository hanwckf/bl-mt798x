#
# Copyright (c) 2025, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#

SALT_TOOL		:=	./tools/mediatek/gen_salt_header/gen_salt_header.sh
SALT_HEADER_PATH	:=	$(APSOC_COMMON)/img_dec/include

define add_key_salt
    KEY_SALTS += $(1)_SALT
endef

define gen_key_salts
    $(shell rm -f $(SALT_HEADER_PATH)/salt.h)
    $(foreach salt,$(KEY_SALTS),$(shell $(SALT_TOOL) $($(salt)) $(salt) $(SALT_HEADER_PATH)/salt.h))
endef

ifneq ($(filter 1,$(FIP_ENC) $(FW_ENC)),)
$(eval $(call add_key_salt,ROE_KEY))

ifeq ($(FIP_ENC),1)
$(eval $(call add_key_salt,FIP_KEY))
endif

ifeq ($(FW_ENC),1)
$(eval $(call add_key_salt,KERNEL_KEY))
$(eval $(call add_key_salt,ROOTFS_KEY))
endif

$(eval $(call gen_key_salts))

endif # FIP_ENC or FW_ENC
