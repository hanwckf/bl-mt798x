#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xz/xz.mk

# BL2 image options
BL2_IS_AARCH64		:=	1
BL2_SIG_TYPE		:=	sha256+rsa-m1pss
BL2_IMG_HDR_SOC		:=	mt7622

BL2_CPPFLAGS		+=	-I$(MTK_PLAT_SOC)/drivers/pinctrl

BL2_SOURCES		+=	common/desc_image_load.c			\
				common/image_decompress.c			\
				drivers/arm/gic/common/gic_common.c		\
				drivers/arm/gic/v2/gicv2_main.c			\
				drivers/arm/gic/v2/gicv2_helpers.c		\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				drivers/gpio/gpio.c				\
				drivers/io/io_storage.c				\
				drivers/io/io_block.c				\
				drivers/io/io_fip.c				\
				lib/cpus/aarch64/cortex_a53.S			\
				$(XZ_SOURCES)					\
				$(APSOC_COMMON)/drivers/uart/aarch64/hsuart.S	\
				$(MTK_PLAT_SOC)/aarch64/plat_helpers.S		\
				$(MTK_PLAT_SOC)/aarch64/platform_common.c	\
				$(MTK_PLAT_SOC)/bl2/bl2_plat_init.c		\
				$(MTK_PLAT_SOC)/drivers/pinctrl/pinctrl.c	\
				$(MTK_PLAT_SOC)/drivers/pll/pll.c		\
				$(MTK_PLAT_SOC)/drivers/pmic/pmic_wrap.c	\
				$(MTK_PLAT_SOC)/drivers/pmic/pmic.c		\
				$(MTK_PLAT_SOC)/drivers/spm/mtcmos.c		\
				$(MTK_PLAT_SOC)/drivers/timer/cpuxgpt.c		\
				$(MTK_PLAT_SOC)/drivers/wdt/mtk_wdt.c		\
				$(MTK_PLAT_SOC)/mtk_ar_table.c

# Include dram driver files
include $(MTK_PLAT_SOC)/drivers/dram/dram.mk

# Trusted board boot
include $(APSOC_COMMON)/bl2/tbbr.mk

# Anti-rollback
include $(MTK_PLAT_SOC)/bl2/ar.mk

ifeq ($(TRUSTED_BOARD_BOOT),1)
BL2_SOURCES		+=	plat/common/tbbr/plat_tbbr.c
endif

ifeq ($(BL2_COMPRESS),1)
BL2_CPPFLAGS		+=	-DUSING_BL2PL
endif # END OF BL2_COMPRESS

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

ifeq ($(BOOT_DEVICE),emmc)
$(eval $(call BL2_BOOT_EMMC))
BL2_SOURCES		+=	$(MTK_PLAT_SOC)/bl2/bl2_dev_mmc.c
BL2_CPPFLAGS		+=	-DMSDC_INDEX=0
endif # END OF BOOTDEVICE = emmc

ifeq ($(BOOT_DEVICE),sdmmc)
$(eval $(call BL2_BOOT_SD))
BL2_SOURCES		+=	$(MTK_PLAT_SOC)/bl2/bl2_dev_mmc.c
CPPFLAGS		+=	-DMSDC_INDEX=1
DEVICE_HEADER_OFFSET	?=	0x80000
endif # END OF BOOTDEVICE = sdmmc

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
