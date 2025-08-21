# SPDX-License-Identifier:	GPL-2.0

obj-y += rootdisk.o

ifndef CONFIG_XPL_BUILD
obj-$(CONFIG_MEDIATEK_BOOTMENU) += load_data.o upgrade_helper.o boot_helper.o \
				   untar.o image_helper.o verify_helper.o \
				   dm_parser.o bootmenu_common.o
obj-$(CONFIG_XZ) += unxz.o cmd_xzdec.o
ifdef CONFIG_MTD
obj-$(CONFIG_MEDIATEK_BOOTMENU) += mtd_helper.o
endif

ifdef CONFIG_MMC
obj-$(CONFIG_MEDIATEK_BOOTMENU) += mmc_helper.o
endif

obj-$(CONFIG_MEDIATEK_MULTI_MTD_LAYOUT) += mtd_layout.o

obj-$(CONFIG_MEDIATEK_BOOTMENU) += cmd_mtkupgrade.o cmd_mtkload.o \
				   cmd_mtkboardboot.o cmd_mtkautoboot.o \
				   cmd_mtkbootconf.o

obj-$(CONFIG_MTK_BOOTMENU_MTD_COMMON) += bootmenu_mtd_common.o
obj-$(CONFIG_MTK_BOOTMENU_MMC_COMMON) += bootmenu_mmc_common.o

obj-$(CONFIG_MTK_BOOTMENU_MTD) += bootmenu_mtd.o
obj-$(CONFIG_MTK_BOOTMENU_UBI) += bootmenu_ubi.o
obj-$(CONFIG_MTK_BOOTMENU_MTD_LEGACY) += bootmenu_mtd_legacy.o
obj-$(CONFIG_MTK_BOOTMENU_MMC) += bootmenu_mmc.o
obj-$(CONFIG_MTK_BOOTMENU_SNOR_EMMC) += bootmenu_snor_emmc.o

obj-$(CONFIG_MTK_BSPCONF_SUPPORT) += bsp_conf.o cmd_bspconf.o
obj-$(CONFIG_MTK_BOARDINFO) += board_info.o
obj-$(CONFIG_MTK_FIP_SUPPORT) += fip.o fip_helper.o cmd_fip.o
obj-$(CONFIG_MTK_UPGRADE_BL2_VERIFY) += bl2_helper.o cmd_bl2.o
obj-$(CONFIG_MTK_UPGRADE_FIP_VERIFY) += fip_helper.o

obj-$(CONFIG_MTK_WEB_FAILSAFE) += failsafe.o

obj-$(CONFIG_MTK_WGET) += mtk_wget.o cmd_wget.o

endif

obj-$(CONFIG_MTK_ANTI_ROLLBACK) += mtk_ar.o mtk_efuse.o

obj-$(CONFIG_MTK_DUAL_BOOT) += dual_boot.o

obj-$(CONFIG_ENABLE_NAND_NMBM) += nmbm.o

obj-$(CONFIG_MTK_MISC_CUSTOMIZED) += customizations/

obj-$(CONFIG_MTK_FW_ENCRYPT_VIA_BL31) += mtk_aes_decrypt.o
obj-$(CONFIG_MTK_FW_ENCRYPT_VIA_OPTEE) += mtk_optee_aes_decrypt.o
