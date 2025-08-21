#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MTK_PLAT		:=	plat/mediatek
MTK_PLAT_SOC		:=	$(MTK_PLAT)/$(PLAT)
APSOC_COMMON		:=	$(MTK_PLAT)/apsoc_common

ifeq ($(FPGA),1)
MTK_PLAT_SOC_BSP	:=	$(MTK_PLAT_SOC)/fpga
include $(MTK_PLAT_SOC_BSP)/fpga.mk
else
MTK_PLAT_SOC_BSP	:=	$(MTK_PLAT_SOC)
endif

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1

ENABLE_PIE		:=	1

# Do not enable SVE
ENABLE_SVE_FOR_NS	:=	0
MULTI_CONSOLE_API	:=	1

RESET_TO_BL2		:=	1

PLAT_INCLUDES		+=	-Iinclude/plat/arm/common			\
				-Iinclude/plat/arm/common/aarch64		\
				-I$(APSOC_COMMON)				\
				-I$(APSOC_COMMON)/drivers/uart			\
				-I$(APSOC_COMMON)/drivers/trng/v2		\
				-I$(APSOC_COMMON)/drivers/wdt			\
				-I$(MTK_PLAT_SOC)/drivers/dram			\
				-I$(MTK_PLAT_SOC)/drivers/pll			\
				-I$(MTK_PLAT_SOC)/drivers/spmc			\
				-I$(MTK_PLAT_SOC)/drivers/timer			\
				-I$(MTK_PLAT_SOC)/drivers/devapc		\
				-I$(MTK_PLAT_SOC)/include

include $(MTK_PLAT_SOC_BSP)/cpu.mk
include $(MTK_PLAT_SOC_BSP)/bl2pl/bl2pl.mk
include $(MTK_PLAT_SOC_BSP)/bl2/bl2.mk
include $(MTK_PLAT_SOC_BSP)/bl31/bl31.mk
include $(MTK_PLAT_SOC_BSP)/bl32.mk
include $(MTK_PLAT_SOC)/drivers/efuse/efuse.mk

include $(APSOC_COMMON)/img_dec/img_dec.mk
include $(APSOC_COMMON)/bl2/tbbr_post.mk
include $(APSOC_COMMON)/bl2/ar_post.mk
include $(APSOC_COMMON)/bl2/bl2_image_post.mk

# Make sure make command parameter reflects on .o files immediately
include make_helpers/dep.mk

$(call GEN_DEP_RULES,bl2,emicfg bl2_boot_ram bl2_boot_nand_nmbm bl2_dev_mmc bl2_plat_init bl2_plat_setup mt7987_gpio dtb)
$(call MAKE_DEP,bl2,emicfg,DDR4_4BG_MODE DRAM_DEBUG_LOG DDR3_FREQ_2133 DDR3_FREQ_1866 DDR4_FREQ_3200 DDR4_FREQ_2666)
$(call MAKE_DEP,bl2,bl2_plat_init,BL2_COMPRESS)
$(call MAKE_DEP,bl2,bl2_plat_setup,BOOT_DEVICE TRUSTED_BOARD_BOOT BL32_TZRAM_BASE BL32_TZRAM_SIZE BL32_LOAD_OFFSET)
$(call MAKE_DEP,bl2,bl2_dev_mmc,BOOT_DEVICE)
$(call MAKE_DEP,bl2,bl2_boot_ram,RAM_BOOT_DEBUGGER_HOOK RAM_BOOT_UART_DL)
$(call MAKE_DEP,bl2,bl2_boot_nand_nmbm,NMBM_MAX_RATIO NMBM_MAX_RESERVED_BLOCKS NMBM_DEFAULT_LOG_LEVEL)
$(call MAKE_DEP,bl2,mt7987_gpio,ENABLE_JTAG)
$(call MAKE_DEP,bl2,dtb,BOOT_DEVICE)

$(call GEN_DEP_RULES,bl31,bl31_plat_setup plat_sip_calls)
$(call MAKE_DEP,bl31,bl31_plat_setup,TRUSTED_BOARD_BOOT)
$(call MAKE_DEP,bl31,plat_sip_calls,FWDL BL32_TZRAM_BASE BL32_TZRAM_SIZE BL32_LOAD_OFFSET)
