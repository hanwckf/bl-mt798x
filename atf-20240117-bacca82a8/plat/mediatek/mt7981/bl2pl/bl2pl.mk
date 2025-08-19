#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# BL2PL for BL2 compression
ifeq ($(BL2_COMPRESS),1)
BL2PL_SOURCES		+=	$(APSOC_COMMON)/bl2pl/udelay.c			\
				$(MTK_PLAT_SOC)/bl2pl/bl2pl_plat_setup.c	\
				$(MTK_PLAT_SOC)/bl2pl/serial.c			\
				$(MTK_PLAT_SOC)/drivers/timer/timer.c		\
				$(MTK_PLAT_SOC)/drivers/pll/pll.c

BL2PL_CPPFLAGS		+=	-DXZ_SIMPLE_PRINT_ERROR
endif # END OF BL2_COMPRESS
