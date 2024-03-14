#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(EMERG_MEM_DUMP),1)
BL31_SOURCES		+=	$(APSOC_COMMON)/bl31/memdump.c			\
				$(APSOC_COMMON)/net_common.c			\
				$(APSOC_COMMON)/drivers/eth/mtk_eth.c		\
				common/tf_crc32.c

BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl31				\
				-I$(APSOC_COMMON)/drivers/eth
BL31_CPPFLAGS		+=	-DEMERG_MEM_DUMP
BL31_CFLAGS		+=	-march=armv8-a+crc

include make_helpers/dep.mk

$(call GEN_DEP_RULES,bl31,memdump)
$(call MAKE_DEP,bl31,memdump,EMERG_MEM_DUMP)

endif
