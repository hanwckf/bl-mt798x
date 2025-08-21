#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

ifeq ($(EMERG_MEM_DUMP),1)
ifeq ($(MTK_ETH_AUTONEG_TIMEOUT),)
MTK_ETH_AUTONEG_TIMEOUT := 15
endif
ifeq ($(EMERG_MEM_DUMP_AUTONEG_TIMEOUT),)
EMERG_MEM_DUMP_AUTONEG_TIMEOUT := 30
endif

BL31_SOURCES		+=	$(APSOC_COMMON)/bl31/memdump.c			\
				$(APSOC_COMMON)/net_common.c			\
				$(APSOC_COMMON)/drivers/eth/mtk_eth.c		\
				common/tf_crc32.c

BL31_CPPFLAGS		+=	-I$(APSOC_COMMON)/bl31				\
				-I$(APSOC_COMMON)/drivers/eth
BL31_CPPFLAGS		+=	-DEMERG_MEM_DUMP				\
				-DMTK_ETH_AUTONEG_TIMEOUT=$(MTK_ETH_AUTONEG_TIMEOUT)\
				-DEMERG_MEM_DUMP_AUTONEG_TIMEOUT=$(EMERG_MEM_DUMP_AUTONEG_TIMEOUT)
BL31_CFLAGS		+=	-march=armv8-a+crc

ifeq ($(MTK_ETH_USE_I2P5G_PHY),1)
BL31_SOURCES		+=	$(APSOC_COMMON)/drivers/eth/phy.c		\
				$(APSOC_COMMON)/drivers/eth/mtk-i2p5ge.c
BL31_CPPFLAGS		+=	-DMTK_ETH_USE_I2P5G_PHY
ifeq ($(MTK_ETH_I2P5G_PHY_FW_LOAD),1)
BL31_CPPFLAGS		+=	-DMTK_ETH_I2P5G_PHY_FW_LOAD
endif
endif

include make_helpers/dep.mk

$(call GEN_DEP_RULES,bl31,memdump mtk_eth mtk-i2p5ge)
$(call MAKE_DEP,bl31,memdump,EMERG_MEM_DUMP EMERG_MEM_DUMP_AUTONEG_TIMEOUT)
$(call MAKE_DEP,bl31,mtk_eth,MTK_ETH_USE_I2P5G_PHY MTK_ETH_AUTONEG_TIMEOUT)
$(call MAKE_DEP,bl31,mtk-i2p5ge,MTK_ETH_I2P5G_PHY_FW_LOAD MTK_ETH_AUTONEG_TIMEOUT)

endif
