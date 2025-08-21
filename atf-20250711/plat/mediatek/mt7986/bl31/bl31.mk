#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xlat_tables_v2/xlat_tables.mk

# Platform defined GICv3
USE_GIC_DRIVER		:=      1
# Include GICv3 driver files
include drivers/arm/gic/v3/gicv3.mk

BL31_SOURCES		+=	drivers/arm/cci/cci.c				\
				$(GICV3_SOURCES)				\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a53.S			\
				plat/common/plat_gicv3.c			\
				$(APSOC_COMMON)/drivers/uart/aarch64/hsuart.S	\
				$(APSOC_COMMON)/drivers/wdt/mtk_wdt.c		\
				$(APSOC_COMMON)/bl31/mtk_sip_svc.c		\
				$(APSOC_COMMON)/bl31/mtk_boot_next.c		\
				$(APSOC_COMMON)/bl31/bl31_common_setup.c	\
				$(APSOC_COMMON)/bl31/apsoc_sip_svc_common.c	\
				$(APSOC_COMMON)/bl31/plat_topology.c		\
				$(APSOC_COMMON)/bl31/mtk_gic_v3.c		\
				$(APSOC_COMMON)/bl31/plat_pm.c			\
				$(APSOC_COMMON)/drivers/trng/v1/rng.c		\
				$(MTK_PLAT_SOC)/aarch64/plat_helpers.S		\
				$(MTK_PLAT_SOC)/aarch64/platform_common.c	\
				$(MTK_PLAT_SOC)/bl31/bl31_plat_setup.c		\
				$(MTK_PLAT_SOC)/bl31/bl31_plat_pm.c		\
				$(MTK_PLAT_SOC)/bl31/plat_sip_calls.c		\
				$(MTK_PLAT_SOC)/drivers/spmc/mtspmc.c		\
				$(MTK_PLAT_SOC)/drivers/timer/timer.c		\
				$(MTK_PLAT_SOC)/drivers/spmc/mtspmc.c		\
				$(MTK_PLAT_SOC)/drivers/dram/emi_mpu.c		\
				$(MTK_PLAT_SOC)/drivers/devapc/devapc.c

BL31_SOURCES		+=	$(XLAT_TABLES_LIB_SRCS)
BL31_CPPFLAGS		+=	-DPLAT_XLAT_TABLES_DYNAMIC
BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl31

include $(APSOC_COMMON)/bl31/memdump.mk
ifeq ($(EMERG_MEM_DUMP),1)
BL31_SOURCES		+=	$(MTK_PLAT_SOC)/drivers/pll/pll.c		\
				$(APSOC_COMMON)/drivers/eth/mt753x.c		\
				$(APSOC_COMMON)/drivers/eth/mt7531.c
endif

ifeq ($(ENABLE_BL31_RUNTIME_LOG), 1)
BL31_CPPFLAGS		+=	-DENABLE_BL31_RUNTIME_LOG
endif

MTK_SIP_KERNEL_BOOT_ENABLE := 1
$(eval $(call add_define,MTK_SIP_KERNEL_BOOT_ENABLE))
