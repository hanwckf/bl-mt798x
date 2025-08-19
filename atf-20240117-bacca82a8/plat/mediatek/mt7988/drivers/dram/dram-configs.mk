#
# Copyright (c) 2022, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Whether supports COMB
ifeq ($(DRAM_USE_COMB), 1)
BL2_CPPFLAGS		+=	-DDRAM_USE_COMB
endif # END OF DRAM_USE_COMB

# Whether supports DDR4x32 8GB
ifeq ($(DDR4_4BG_MODE), 1)
BL2_CPPFLAGS		+=	-DDDR4_4BG_MODE
endif # END OF DDR4_4BG_MODE

# Whether supports DDR4
ifeq ($(DRAM_USE_DDR4), 1)
BL2_CPPFLAGS		+=	-DDRAM_USE_DDR4
AVAIL_DRAM_SIZE		:=	1024 2048
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

# Whether to display verbose DRAM log
ifeq ($(DRAM_DEBUG_LOG), 1)
BL2_CPPFLAGS		+=	-DDRAM_DEBUG_LOG
endif # END OF DRAM_DEBUG_LOG

# DDR3 frequency
ifeq ($(DDR3_FREQ_2133), 1)
BL2_CPPFLAGS		+=	-DDDR3_FREQ_2133
endif
ifeq ($(DDR3_FREQ_1866), 1)
BL2_CPPFLAGS		+=	-DDDR3_FREQ_1866
endif # END OF DDR3_FREQ_xxxx

# DDR4 frequency
ifeq ($(DDR4_FREQ_3200), 1)
BL2_CPPFLAGS		+=	-DDDR4_FREQ_3200
endif
ifeq ($(DDR4_FREQ_2666), 1)
BL2_CPPFLAGS		+=	-DDDR4_FREQ_2666
endif # END OF DDR4_FREQ_xxxx

# DDR refresh interval
ifeq ($(DDR_REFRESH_INTERVAL_780), 1)
BL2_CPPFLAGS		+=	-DDDR_REFRESH_INTERVAL_780
endif
# DDR refresh interval
ifeq ($(DDR_REFRESH_INTERVAL_390), 1)
BL2_CPPFLAGS		+=	-DDDR_REFRESH_INTERVAL_390
endif
# DDR refresh interval
ifeq ($(DDR_REFRESH_INTERVAL_292), 1)
BL2_CPPFLAGS		+=	-DDDR_REFRESH_INTERVAL_292
endif
# DDR refresh interval
ifeq ($(DDR_REFRESH_INTERVAL_195), 1)
BL2_CPPFLAGS		+=	-DDDR_REFRESH_INTERVAL_195
endif