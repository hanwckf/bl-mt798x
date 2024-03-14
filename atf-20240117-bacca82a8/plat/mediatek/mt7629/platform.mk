#
# Copyright (c) 2020, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MTK_PLAT		:=	plat/mediatek
MTK_PLAT_SOC		:=	$(MTK_PLAT)/$(PLAT)
APSOC_COMMON		:=	$(MTK_PLAT)/apsoc_common

# Not needed for Cortex-A7
WORKAROUND_CVE_2017_5715	:=	0

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1

# Do not enable SVE
ENABLE_SVE_FOR_NS	:=	0
MULTI_CONSOLE_API	:=	1

# AArch32-specific
AARCH32_SP		:=	sp_min
ARM_CORTEX_A7		:=	yes
ARM_ARCH_MAJOR		:=	7
MARCH32_DIRECTIVE	:=	-mcpu=cortex-a7 -msoft-float
RESET_TO_BL2		:=	1

PLAT_INCLUDES		:=	-I$(APSOC_COMMON)				\
				-I$(APSOC_COMMON)/drivers/uart			\
				-I$(APSOC_COMMON)/drivers/efuse			\
				-I$(APSOC_COMMON)/drivers/efuse/include		\
				-Iinclude/plat/arm/common			\
				-I$(MTK_PLAT_SOC)/drivers/pll			\
				-I$(MTK_PLAT_SOC)/drivers/spm			\
				-I$(MTK_PLAT_SOC)/drivers/timer			\
				-I$(MTK_PLAT_SOC)/drivers/wdt			\
				-I$(MTK_PLAT_SOC)/include

PLAT_BL_COMMON_SOURCES	:=	lib/cpus/aarch32/cortex_a7.S			\
				$(APSOC_COMMON)/drivers/uart/aarch32/hsuart.S	\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				$(MTK_PLAT_SOC)/aarch32/plat_helpers.S		\
				$(MTK_PLAT_SOC)/aarch32/platform_common.c

CPPFLAGS		+=	-D__SOFTFP__

include $(MTK_PLAT_SOC)/bl2/bl2.mk
include $(MTK_PLAT_SOC)/sp_min/bl32.mk
include $(MTK_PLAT_SOC)/drivers/efuse/efuse.mk

include $(APSOC_COMMON)/bl2/tbbr_post.mk
include $(APSOC_COMMON)/bl2/bl2_image_post.mk

# Make sure make command parameter takes effect on .o files immediately
include make_helpers/dep.mk

$(call GEN_DEP_RULES,bl2,bl2_boot_ram bl2_boot_nand_nmbm mtk_efuse bl2_plat_init)
$(call MAKE_DEP,bl2,bl2_plat_init,BL2_COMPRESS)
$(call MAKE_DEP,bl2,bl2_boot_ram,RAM_BOOT_DEBUGGER_HOOK RAM_BOOT_UART_DL)
$(call MAKE_DEP,bl2,bl2_boot_nand_nmbm,NMBM_MAX_RATIO NMBM_MAX_RESERVED_BLOCKS NMBM_DEFAULT_LOG_LEVEL)
$(call MAKE_DEP,bl2,mtk_efuse,TRUSTED_BOARD_BOOT)

$(call GEN_DEP_RULES,bl32,mtk_efuse)
$(call MAKE_DEP,bl32,mtk_efuse,TRUSTED_BOARD_BOOT)
