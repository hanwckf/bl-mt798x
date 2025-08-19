#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

BL32_SOURCES		+=	drivers/arm/cci/cci.c				\
				drivers/arm/gic/common/gic_common.c		\
				drivers/arm/gic/v2/gicv2_main.c			\
				drivers/arm/gic/v2/gicv2_helpers.c		\
				plat/common/plat_gicv2.c			\
				plat/common/plat_psci_common.c			\
				$(APSOC_COMMON)/bl31/mtk_sip_svc.c		\
				$(APSOC_COMMON)/bl31/apsoc_sip_svc_common.c	\
				$(APSOC_COMMON)/bl31/plat_topology.c		\
				$(APSOC_COMMON)/bl31/mtk_gic_v2.c		\
				$(APSOC_COMMON)/drivers/efuse/mtk_efuse.c	\
				$(MTK_PLAT_SOC)/drivers/spm/spmc.c		\
				$(MTK_PLAT_SOC)/drivers/wdt/mtk_wdt.c		\
				$(MTK_PLAT_SOC)/sp_min/plat_sip_calls.c		\
				$(MTK_PLAT_SOC)/sp_min/plat_pm.c

BL32_SOURCES		+=	lib/xlat_tables/xlat_tables_common.c		\
				lib/xlat_tables/aarch32/xlat_tables.c

BL32_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl31
