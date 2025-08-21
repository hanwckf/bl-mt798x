#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include lib/xlat_tables_v2/xlat_tables.mk
# Platform defined GICv2
USE_GIC_DRIVER		:=      1
include drivers/arm/gic/v2/gicv2.mk

BL32_SOURCES		+=	drivers/arm/cci/cci.c				\
				$(GICV2_SOURCES)				\
				plat/common/plat_gicv2.c			\
				plat/common/plat_psci_common.c			\
				$(APSOC_COMMON)/bl31/mtk_sip_svc.c		\
				$(APSOC_COMMON)/bl31/apsoc_sip_svc_common.c	\
				$(APSOC_COMMON)/bl31/plat_topology.c		\
				$(APSOC_COMMON)/bl31/mtk_gic_v2.c		\
				$(MTK_PLAT_SOC)/drivers/spm/spmc.c		\
				$(MTK_PLAT_SOC)/drivers/wdt/mtk_wdt.c		\
				$(MTK_PLAT_SOC)/sp_min/plat_sip_calls.c		\
				$(MTK_PLAT_SOC)/sp_min/plat_pm.c

BL32_SOURCES		+=	$(XLAT_TABLES_LIB_SRCS)
BL32_CPPFLAGS		+=	-DPLAT_XLAT_TABLES_DYNAMIC
BL32_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl31
