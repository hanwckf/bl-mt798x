#
# Copyright (c) 2025, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

PLAT_INCLUDES		+=	-I$(APSOC_COMMON)/drivers/efuse			\
				-I$(APSOC_COMMON)/drivers/efuse/include

BL_COMMON_SOURCES	+=	$(APSOC_COMMON)/drivers/efuse/mtk_efuse.c

MTK_EFUSE_PRESENT := 1
$(eval $(call add_define,MTK_EFUSE_PRESENT))
