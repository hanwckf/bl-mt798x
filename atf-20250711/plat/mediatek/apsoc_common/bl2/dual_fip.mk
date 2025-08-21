#
# Copyright (c) 2025, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Dual-FIP feature
#
ifeq ($(DUAL_FIP),1)

BL2_CPPFLAGS		+=	-DDUAL_FIP

BL2_SOURCES		+=	common/tf_crc32.c				\
				$(APSOC_COMMON)/bl2/bsp_conf.c			\
				$(APSOC_COMMON)/bl2/dual_fip.c

ifneq ($(TRUSTED_BOARD_BOOT),1)
BL2_SOURCES		+=	$(APSOC_COMMON)/bl2/sha256/sha256.c
endif

ifeq ($(FIP_IN_BOOT0), 1)
BL2_CPPFLAGS		+=	-DFIP_IN_BOOT0
endif

ifeq ($(FIP2_IN_BOOT1), 1)
BL2_CPPFLAGS		+=	-DFIP2_IN_BOOT1
endif

endif

include make_helpers/dep.mk

$(call GEN_DEP_RULES,bl2,bl2_image_load_v2 bsp_conf dual_fip bl2_boot_nand_ubi bl2_boot_mmc bl2_plat_setup)
$(call MAKE_DEP,bl2,bl2_image_load_v2,DUAL_FIP)
$(call MAKE_DEP,bl2,bsp_conf,LOG_LEVEL)
$(call MAKE_DEP,bl2,dual_fip,LOG_LEVEL NEED_BL32 TRUSTED_BOARD_BOOT)
$(call MAKE_DEP,bl2,bl2_boot_nand_ubi,DUAL_FIP)
$(call MAKE_DEP,bl2,bl2_boot_mmc,DUAL_FIP FIP_IN_BOOT0 FIP2_IN_BOOT1)
$(call MAKE_DEP,bl2,bl2_plat_setup,DUAL_FIP)
