# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
# Auther: Weijie Gao <weijie.gao@mediatek.com>
#
# Script for Kconfig, pre-configuration
#

_STRING_CONFIGS := PLAT ARCH BOOT_DEVICE NAND_TYPE BL33 BL32 SPD MBEDTLS_DIR \
		   ROT_KEY ROT_PUB_KEY BROM_SIGN_KEY ANTI_ROLLBACK_TABLE \
		   AR_TABLE_XML TOPS_KEY_HASH WO_KEY_HASH CROSS_COMPILE

ifeq ($(BUILD_BASE),)
	KCONFIG_BUILD_BASE	:=	./build
else
	KCONFIG_BUILD_BASE	:=	$(BUILD_BASE)
endif

ifneq ("$(wildcard $(KCONFIG_BUILD_BASE)/.config)", "")
ifeq ($(filter defconfig menuconfig savedefconfig %_defconfig %_defconfig_update %_defconfig_refresh,$(MAKECMDGOALS)),)
include $(KCONFIG_BUILD_BASE)/.config
$(foreach cfg,$(_STRING_CONFIGS),$(if $(value $(cfg)),$(eval $(cfg):=$$(patsubst "%",%,$(value $(cfg))))))
endif
endif
