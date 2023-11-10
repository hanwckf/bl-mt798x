#
# Copyright (c) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

include plat/mediatek/mt7988/drivers/dram/dram-configs.mk

PREBUILT_LIBS		+=	${MTK_PLAT_SOC}/drivers/dram/release/dram.o
BL2_SOURCES		+=	${MTK_PLAT_SOC}/drivers/dram/emicfg.c
PLAT_INCLUDES		+=	-I${MTK_PLAT_SOC}/drivers/dram
