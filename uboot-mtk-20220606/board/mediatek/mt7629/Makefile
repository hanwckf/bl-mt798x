# SPDX-License-Identifier:	GPL-2.0

obj-y += mt7629_rfb.o
ccflags-y += -I$(srctree)/board/mediatek/common

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_MT7629_BOOTMENU_SNOR) += bootmenu_snor.o
obj-$(CONFIG_MT7629_BOOTMENU_SNOR_SB) += bootmenu_snor_sb.o
obj-$(CONFIG_MT7629_BOOTMENU_SNAND) += bootmenu_snand.o
obj-$(CONFIG_MT7629_BOOTMENU_SNAND_SB) += bootmenu_snand_sb.o
endif
