#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Whether supports DDR4
ifeq ($(DRAM_USE_DDR4), 1)
BL2_CPPFLAGS		+=	-DDRAM_USE_DDR4
AVAIL_DRAM_SIZE		:=	512 1024
else
override DRAM_USE_DDR4	:=	0
AVAIL_DRAM_SIZE		:=	256 512
endif # END OF DRAM_USE_DDR4

# Whether to limit the DRAM size
ifdef DRAM_SIZE_LIMIT
ifneq ($(filter $(DRAM_SIZE_LIMIT),$(AVAIL_DRAM_SIZE)),)
BL2_CPPFLAGS		+=	-DDRAM_SIZE_LIMIT=$(DRAM_SIZE_LIMIT)
else
$(error You must specify the correct DRAM_SIZE_LIMIT. Avaliable values: $(AVAIL_DRAM_SIZE))
endif # END OF DRAM_SIZE_LIMIT check
else
DRAM_SIZE_LIMIT		:= 	0
endif # END OF DRAM_SIZE_LIMIT

# DDR3 frequency
ifeq ($(DDR3_FREQ_2133), 1)
BL2_CPPFLAGS		+=	-DDDR3_FREQ_2133
endif
ifeq ($(DDR3_FREQ_1866), 1)
BL2_CPPFLAGS		+=	-DDDR3_FREQ_1866
endif # END OF DDR3_FREQ_xxxx

# Whether to display verbose DRAM log
ifeq ($(DRAM_DEBUG_LOG), 1)
BL2_CPPFLAGS		+=	-DDRAM_DEBUG_LOG
endif # END OF DRAM_DEBUG_LOG

# QFN board type
ifeq ($(BOARD_QFN), 1)
BL2_CPPFLAGS		+=	-DBOARD_QFN
endif # END OF BOARD_QFN

# BGA board type
ifeq ($(BOARD_BGA), 1)
BL2_CPPFLAGS		+=	-DBOARD_BGA
endif # END OF BOARD_BGA
