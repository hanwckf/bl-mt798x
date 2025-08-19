#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

define BL2_FIP_OVERRIDE_COMMON
ifneq ($(OVERRIDE_FIP_BASE),)
BL2_CPPFLAGS		+=	-DOVERRIDE_FIP_BASE=$(OVERRIDE_FIP_BASE)
endif
ifneq ($(OVERRIDE_FIP_SIZE),)
BL2_CPPFLAGS		+=	-DOVERRIDE_FIP_SIZE=$(OVERRIDE_FIP_SIZE)
endif
endef

define BL2_BOOT_COMMON
BL2_SOURCES		+=	$(APSOC_COMMON)/bl2/bl2_plat_setup.c
BL2_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl2
endef

define BL2_BOOT_RAM
$(call BL2_BOOT_COMMON)
BL2_SOURCES		+=	drivers/io/io_memmap.c				\
				$(APSOC_COMMON)/bl2/bl2_boot_ram.c
BROM_HEADER_TYPE	:=	nor
ifeq ($(RAM_BOOT_DEBUGGER_HOOK), 1)
BL2_CPPFLAGS		+=	-DRAM_BOOT_DEBUGGER_HOOK
endif
ifeq ($(RAM_BOOT_UART_DL), 1)
ENABLE_CONSOLE_GETC	:=	1
BL2_SOURCES		+=	$(APSOC_COMMON)/bl2/uart_dl.c
BL2_CPPFLAGS		+=	-DRAM_BOOT_UART_DL
endif
endef # End of BL2_BOOT_RAM

define BL2_BOOT_NOR_MMAP
$(call BL2_BOOT_COMMON)
$(call BL2_FIP_OVERRIDE_COMMON)
BL2_SOURCES		+=	drivers/io/io_memmap.c				\
				$(APSOC_COMMON)/bl2/bl2_boot_nor_mmap.c
BROM_HEADER_TYPE	:=	nor
endef # End of BL2_BOOT_NOR_MMAP

define BL2_BOOT_NOR
$(call BL2_BOOT_COMMON)
$(call BL2_FIP_OVERRIDE_COMMON)
BL2_SOURCES		+=	drivers/mtd/nor/spi_nor.c			\
				drivers/mtd/spi-mem/spi_mem.c			\
				$(APSOC_COMMON)/bl2/bl2_boot_nor.c		\
				$(APSOC_COMMON)/bl2/bl2_dev_spi_nor_init.c
BL2_CPPFLAGS		+=	-Iinclude/lib/libfdt
BL2_CPPFLAGS		+=	-DMTK_SPIM_NOR
BROM_HEADER_TYPE	:=	nor
endef # End of BL2_BOOT_NOR

define BL2_BOOT_MMC
$(call BL2_BOOT_COMMON)
BL2_SOURCES		+=	drivers/mmc/mmc.c				\
				drivers/partition/partition.c			\
				drivers/partition/gpt.c				\
				common/tf_crc32.c				\
				$(APSOC_COMMON)/drivers/mmc/mtk-sd.c		\
				$(APSOC_COMMON)/bl2/bl2_boot_mmc.c
BL2_CPPFLAGS		+=	-DMTK_MMC_BOOT
BL2_CFLAGS		+=	-march=armv8-a+crc
endef # End of BL2_BOOT_MMC

define BL2_BOOT_EMMC
$(call BL2_BOOT_MMC)
BROM_HEADER_TYPE	:=	emmc
endef # End of BL2_BOOT_EMMC

define BL2_BOOT_SD
$(call BL2_BOOT_MMC)
BROM_HEADER_TYPE	:=	sdmmc
endef # End of BL2_BOOT_SD

#
# $(1): Default enable NMBM
# $(2): Default enable UBI
define BL2_NAND_COMMON
ifeq ($$(NMBM),)
ifeq ($(1),1)
NMBM := 1
endif
endif

ifeq ($$(UBI),)
ifeq ($(2),1)
UBI := 1
endif
endif

ifeq ($$(NMBM)$$(UBI),11)
$$(error NMBM and UBI cannot be enabled together.)
endif

ifeq ($$(NMBM),1)
include lib/nmbm/nmbm.mk
BL2_SOURCES		+=	$$(NMBM_SOURCES)				\
				$(APSOC_COMMON)/bl2/bl2_boot_nand_nmbm.c
ifneq ($(NMBM_MAX_RATIO),)
BL2_CPPFLAGS		+=	-DNMBM_MAX_RATIO=$(NMBM_MAX_RATIO)
endif
ifneq ($(NMBM_MAX_RESERVED_BLOCKS),)
BL2_CPPFLAGS		+=	-DNMBM_MAX_RESERVED_BLOCKS=$(NMBM_MAX_RESERVED_BLOCKS)
endif
ifneq ($(NMBM_DEFAULT_LOG_LEVEL),)
BL2_CPPFLAGS		+=	-DNMBM_DEFAULT_LOG_LEVEL=$(NMBM_DEFAULT_LOG_LEVEL)
endif
endif

ifeq ($$(UBI),1)
BL2_SOURCES		+=	drivers/io/ubi/io_ubi.c				\
				drivers/io/ubi/ubispl.c				\
				$(APSOC_COMMON)/bl2/bl2_boot_nand_ubi.c
BL2_CPPFLAGS		+=	-Idrivers/io/ubi
ifneq (${ARCH},aarch32)
BL2_CFLAGS		+=	-march=armv8-a+crc
else
BL2_SOURCES		+=	drivers/io/ubi/crc32.c
endif
endif

ifneq ($$(NMBM),1)
ifneq ($$(UBI),1)
BL2_SOURCES		+=	$(APSOC_COMMON)/bl2/bl2_boot_nand.c
endif
endif

$(call BL2_BOOT_COMMON)

ifeq ($$(UBI),1)
ifneq ($(OVERRIDE_UBI_START_ADDR),)
BL2_CPPFLAGS		+=	-DOVERRIDE_UBI_START_ADDR=$(OVERRIDE_UBI_START_ADDR)
endif
ifneq ($(OVERRIDE_UBI_END_ADDR),)
BL2_CPPFLAGS		+=	-DOVERRIDE_UBI_END_ADDR=$(OVERRIDE_UBI_END_ADDR)
endif
else
$(call BL2_FIP_OVERRIDE_COMMON)
endif

endef # End of BL2_NAND_COMMON

#
# $(1): Default enable NMBM
# $(2): Default enable UBI
define BL2_BOOT_SNFI
$(call BL2_NAND_COMMON,$(1),$(2))
include $(APSOC_COMMON)/drivers/snfi/mtk-snand.mk
BL2_SOURCES		+=	$$(MTK_SNAND_SOURCES)				\
				$(APSOC_COMMON)/bl2/bl2_dev_snfi_init.c		\
				drivers/mtd/nand/core.c
BL2_CPPFLAGS		+=	-I$(APSOC_COMMON)/drivers/snfi
BL2_CPPFLAGS		+=	-DPRIVATE_MTK_SNAND_HEADER
BROM_HEADER_TYPE	:=	snand
endef # End of BL2_BOOT_SNFI

#
# $(1): Default enable NMBM
# $(2): Default enable UBI
define BL2_BOOT_SPI_NAND
$(call BL2_NAND_COMMON,$(1),$(2))
BL2_SOURCES		+=	drivers/mtd/nand/core.c				\
				$(APSOC_COMMON)/drivers/spi_nand/mtk_spi_nand.c	\
				drivers/mtd/spi-mem/spi_mem.c			\
				$(APSOC_COMMON)/bl2/bl2_dev_spi_nand_init.c
BL2_CPPFLAGS		+=	-Iinclude/lib/libfdt
BL2_CPPFLAGS		+=	-I$(APSOC_COMMON)/drivers/spi_nand
BROM_HEADER_TYPE	:=	spim-nand
endef # End of BL2_BOOT_SPI_NAND

#
# $(1): Type to be checked
# $(2): Available types
define BL2_BOOT_NAND_TYPE_CHECK
ifneq ($(strip $(filter-out $(2),$(1))),)
$$(error NAND header type $(1) is not valid)
endif
endef # End of BL2_BOOT_NAND_TYPE_CHECK
