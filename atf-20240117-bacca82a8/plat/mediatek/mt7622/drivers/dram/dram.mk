#
# Copyright (c) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include $(MTK_PLAT_SOC)/drivers/dram/dram-configs.mk

BL2_CPPFLAGS		+=	-I$(MTK_PLAT_SOC)/drivers/dram

ifeq ($(DDR3_FLYBY), 1)
PREBUILT_LIBS		+=	$(MTK_PLAT_SOC)/drivers/dram/release/dram-flyby.o
else
PREBUILT_LIBS		+=	$(MTK_PLAT_SOC)/drivers/dram/release/dram.o
endif
