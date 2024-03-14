#
# Copyright (c) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

BL2_CPPFLAGS		+=	-I$(MTK_PLAT_SOC)/drivers/dram

PREBUILT_LIBS		+=	$(MTK_PLAT_SOC)/drivers/dram/release/dram.o
