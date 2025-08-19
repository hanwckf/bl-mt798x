#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(ANTI_ROLLBACK),1)

ifneq ($(TRUSTED_BOARD_BOOT),1)
$(error You must enable TRUSTED_BOARD_BOOT when ANTI_ROLLBACK enabled)
endif

ifeq ($(ANTI_ROLLBACK_TABLE),)
$(error You must specify ANTI_ROLLBACK_TABLE when ANTI_ROLLBACK enabled)
endif

AUTO_AR_VER		:=	$(MTK_PLAT_SOC)/auto_ar_ver.c

PLAT_BL_COMMON_SOURCES	+=	$(AUTO_AR_VER)					\
				$(APSOC_COMMON)/mtk_ar.c

CPPFLAGS		+=	-DMTK_ANTI_ROLLBACK

endif
