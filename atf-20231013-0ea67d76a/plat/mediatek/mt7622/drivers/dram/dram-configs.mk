#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(DDR3_FLYBY), 1)
BL2_CPPFLAGS		+=	-DDDR3_FLYBY
endif
