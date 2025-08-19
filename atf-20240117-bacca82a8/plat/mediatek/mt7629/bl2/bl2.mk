#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xz/xz.mk

# BL2 image options
BL2_SIG_TYPE		:=	sha256+rsa-m1pss
BL2_IMG_HDR_SOC		:=	mt7629

BL2_CPPFLAGS		+=	-I$(MTK_PLAT_SOC)/drivers/pinctrl

BL2_SOURCES		+=	common/desc_image_load.c			\
				common/image_decompress.c			\
				drivers/gpio/gpio.c				\
				drivers/io/io_storage.c				\
				drivers/io/io_block.c				\
				drivers/io/io_fip.c				\
				$(XZ_SOURCES)					\
				$(MTK_PLAT_SOC)/bl2/bl2_plat_init.c		\
				${MTK_PLAT_SOC}/drivers/pinctrl/pinctrl.c	\
				${MTK_PLAT_SOC}/drivers/pll/pll.c		\
				${MTK_PLAT_SOC}/drivers/spm/spmc.c		\
				${MTK_PLAT_SOC}/drivers/timer/timer.c		\
				${MTK_PLAT_SOC}/drivers/wdt/mtk_wdt.c

# Include dram driver files
include $(MTK_PLAT_SOC)/drivers/dram/dram.mk

# Trusted board boot
include $(APSOC_COMMON)/bl2/tbbr.mk

ifeq ($(TRUSTED_BOARD_BOOT),1)
BL2_SOURCES		+=	plat/common/tbbr/plat_tbbr.c
endif

# Use aarch32-specific image definitions
BL2_CPPFLAGS		+=	-DMTK_PLAT_NO_DEFAULT_BL2_NEXT_IMAGES

BL2_BASE		:=	0x201000
BL2_CPPFLAGS		+=	-DBL2_BASE=$(BL2_BASE)

#
# Boot device related build macros
#
include $(APSOC_COMMON)/bl2/bl2_image.mk

ifndef BOOT_DEVICE
$(echo You must specify the boot device by provide BOOT_DEVICE= to \
	make parameter. Avaliable values: nor emmc sdmmc snand)
else # NO BOOT_DEVICE

ifeq ($(BOOT_DEVICE),ram)
$(eval $(call BL2_BOOT_RAM))
endif # END OF BOOT_DEVICE = ram

ifeq ($(BOOT_DEVICE),nor)
$(eval $(call BL2_BOOT_NOR_MMAP))
BL2_SOURCES		+=	$(MTK_PLAT_SOC)/bl2/bl2_dev_spi_nor.c
endif # END OF BOOTDEVICE = nor

ifeq ($(BOOT_DEVICE),snand)
$(eval $(call BL2_BOOT_SNFI,0,0))
BL2_SOURCES		+=	$(MTK_PLAT_SOC)/bl2/bl2_dev_snfi.c
NAND_TYPE		?=	2k+64
$(eval $(call BL2_BOOT_NAND_TYPE_CHECK,$(NAND_TYPE),2k+64 2k+120 2k+128 4k+256))
endif # END OF BOOTDEVICE = snand (snfi)

ifeq ($(BROM_HEADER_TYPE),)
$(error BOOT_DEVICE has invalid value. Please re-check.)
endif

endif # END OF BOOT_DEVICE
