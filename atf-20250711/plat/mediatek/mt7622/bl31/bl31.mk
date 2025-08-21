#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xlat_tables_v2/xlat_tables.mk
# Platform defined GICv2
USE_GIC_DRIVER		:=      1
include drivers/arm/gic/v2/gicv2.mk

BL31_SOURCES		+=	drivers/arm/cci/cci.c				\
				$(GICV2_SOURCES)				\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a53.S			\
				$(APSOC_COMMON)/drivers/uart/aarch64/hsuart.S	\
				$(APSOC_COMMON)/bl31/mtk_sip_svc.c		\
				$(APSOC_COMMON)/bl31/mtk_boot_next.c		\
				$(APSOC_COMMON)/bl31/bl31_common_setup.c	\
				$(APSOC_COMMON)/bl31/apsoc_sip_svc_common.c	\
				$(APSOC_COMMON)/bl31/plat_topology.c		\
				$(APSOC_COMMON)/bl31/mtk_gic_v2.c		\
				$(MTK_PLAT_SOC)/aarch64/plat_helpers.S		\
				$(MTK_PLAT_SOC)/aarch64/platform_common.c	\
				$(MTK_PLAT_SOC)/bl31/bl31_plat_setup.c		\
				$(MTK_PLAT_SOC)/drivers/pmic/pmic_wrap.c	\
				$(MTK_PLAT_SOC)/drivers/rtc/rtc.c		\
				$(MTK_PLAT_SOC)/drivers/spm/mtcmos.c		\
				$(MTK_PLAT_SOC)/drivers/spm/spm.c		\
				$(MTK_PLAT_SOC)/drivers/spm/spm_hotplug.c	\
				$(MTK_PLAT_SOC)/drivers/spm/spm_mcdi.c		\
				$(MTK_PLAT_SOC)/drivers/spm/spm_suspend.c	\
				$(MTK_PLAT_SOC)/drivers/timer/cpuxgpt.c		\
				$(MTK_PLAT_SOC)/drivers/wdt/mtk_wdt.c		\
				$(MTK_PLAT_SOC)/bl31/plat_pm.c			\
				$(MTK_PLAT_SOC)/bl31/plat_sip_calls.c		\
				$(MTK_PLAT_SOC)/bl31/power_tracer.c		\
				$(MTK_PLAT_SOC)/bl31/scu.c			\
				$(MTK_PLAT_SOC)/mtk_ar_table.c

BL31_SOURCES		+=	$(XLAT_TABLES_LIB_SRCS)				\
				plat/common/plat_gicv2.c
BL31_CPPFLAGS		+=	-DPLAT_XLAT_TABLES_DYNAMIC
BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl31

ifeq ($(ENABLE_BL31_RUNTIME_LOG), 1)
BL31_CPPFLAGS		+=	-DENABLE_BL31_RUNTIME_LOG
endif
