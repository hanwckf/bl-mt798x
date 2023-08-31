#
# Copyright (C) 2020 MediaTek Inc. All rights reserved.
# Author: Weijie Gao <weijie.gao@mediatek.com>
#
# SPDX-License-Identifier: GPL-2.0
#

obj-y += mtk-snand.o mtk-snand-ecc.o mtk-snand-ids.o mtk-snand-os.o
obj-$(CONFIG_MTK_SPI_NAND_MTD) += mtk-snand-mtd.o

ifdef CONFIG_SPL_BUILD
obj-$(CONFIG_SPL_MTK_SPI_NAND) += mtk-snand-spl.o
endif

ccflags-y += -DPRIVATE_MTK_SNAND_HEADER
