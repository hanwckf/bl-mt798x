#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# efuse related build macros
#

include $(APSOC_COMMON)/drivers/efuse/efuse.mk

PLAT_INCLUDES		+=	-I$(MTK_PLAT_SOC)/drivers/efuse

PREBUILT_LIBS		+=	$(MTK_PLAT_SOC)/drivers/efuse/release/plat_efuse.o	\
				$(MTK_PLAT_SOC)/drivers/efuse/release/efuse_cmd.o
