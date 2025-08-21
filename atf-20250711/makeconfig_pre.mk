# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
#
# Script for Kconfig, pre-configuration
#

CROSS_COMPILE ?= aarch64-linux-gnu-
_STRING_CONFIGS := PLAT ARCH BOOT_DEVICE NAND_TYPE BL33 BL32 SPD MBEDTLS_DIR \
		   ROT_KEY BROM_SIGN_KEY ANTI_ROLLBACK_TABLE \
		   AR_TABLE_XML TOPS_KEY_HASH WO_KEY_HASH CROSS_COMPILE \
		   SCP_BL2_KEY TRUSTED_WORLD_KEY NON_TRUSTED_WORLD_KEY \
		   BL31_KEY BL32_KEY BL33_KEY KEY_ALG KEY_SIZE HASH_ALG \
		   DECRYPTION_SUPPORT \
		   ENC_KEY_PATH ENC_NONCE_PATH \
		   ROE_KEY_SALT ROOTFS_KEY_SALT KERNEL_KEY_SALT FIP_KEY_SALT

ifeq ($(BUILD_BASE),)
	KCONFIG_BUILD_BASE	:=	./build
else
	KCONFIG_BUILD_BASE	:=	$(BUILD_BASE)
endif

ifneq ("$(wildcard $(KCONFIG_BUILD_BASE)/.config)", "")
ifeq ($(filter defconfig menuconfig savedefconfig oldconfig olddefconfig \
	%_defconfig %_defconfig_update %_defconfig_refresh,$(MAKECMDGOALS)),)
include $(KCONFIG_BUILD_BASE)/.config
$(foreach cfg,$(_STRING_CONFIGS),$(if $(value $(cfg)),$(eval $(cfg):=$$(patsubst "%",%,$(value $(cfg))))))
endif
endif
