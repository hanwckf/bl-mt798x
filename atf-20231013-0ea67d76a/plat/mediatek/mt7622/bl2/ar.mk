#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(ANTI_ROLLBACK),1)

ifndef AR_TABLE_XML
$(error You must specify the anti-rollback table XML file by build option \
	AR_TABLE_XML=<path to ar_table.xml>)
endif

AUTO_AR_TABLE		:=	$(MTK_PLAT_SOC)/auto_ar_table.c
AUTO_AR_CONF		:=	$(MTK_PLAT_SOC)/auto_ar_conf.mk

TFW_NVCTR_VAL		?=	0
NTFW_NVCTR_VAL		?=	0
BL_AR_VER		?=	0

PLAT_BL_COMMON_SOURCES	+=	$(AUTO_AR_TABLE)

CPPFLAGS		+=	-DANTI_ROLLBACK

endif
