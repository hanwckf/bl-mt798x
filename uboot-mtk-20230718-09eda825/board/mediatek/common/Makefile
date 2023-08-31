# SPDX-License-Identifier:	GPL-2.0

# Empty file to make sure compilation pass
obj-y += empty.o

ifndef CONFIG_SPL_BUILD
obj-$(CONFIG_MEDIATEK_BOOTMENU) += load_data.o upgrade_helper.o boot_helper.o \
				   untar.o image_helper.o verify_helper.o \
				   dm_parser.o
ifdef CONFIG_MTD
obj-$(CONFIG_MEDIATEK_BOOTMENU) += mtd_helper.o
endif

ifdef CONFIG_MMC
obj-$(CONFIG_MEDIATEK_BOOTMENU) += mmc_helper.o
endif

obj-$(CONFIG_MEDIATEK_BOOTMENU) += cmd_mtkupgrade.o cmd_mtkload.o \
				   cmd_mtkboardboot.o cmd_mtkautoboot.o

obj-$(CONFIG_MTK_BOOTMENU_MTD) += bootmenu_mtd.o
obj-$(CONFIG_MTK_BOOTMENU_MTD_LEGACY) += bootmenu_mtd_legacy.o
obj-$(CONFIG_MTK_BOOTMENU_MMC) += bootmenu_mmc.o

obj-$(CONFIG_MTK_BOARDINFO) += board_info.o
obj-$(CONFIG_MTK_FIP_SUPPORT) += fip.o fip_helper.o cmd_fip.o
obj-$(CONFIG_MTK_UPGRADE_BL2_VERIFY) += bl2_helper.o cmd_bl2.o
obj-$(CONFIG_MTK_UPGRADE_FIP_VERIFY) += fip_helper.o

obj-$(CONFIG_MTK_WEB_FAILSAFE) += failsafe.o

endif

obj-$(CONFIG_MTK_ANTI_ROLLBACK) += mtk_ar.o mtk_efuse.o

obj-$(CONFIG_MTK_DUAL_BOOT) += dual_boot.o

obj-$(CONFIG_ENABLE_NAND_NMBM) += nmbm.o

obj-$(CONFIG_MTK_MISC_CUSTOMIZED) += customizations/

obj-$(CONFIG_MTK_SEC_ENV_MTD) += mtk_sec_env_mtd.o
obj-$(CONFIG_MTK_SEC_ENV_MMC) += mtk_sec_env_mmc.o
obj-$(CONFIG_MTK_FSEK) += mtk_sec_env.o
obj-$(CONFIG_MTK_FSEK) += mtk_fsek.o
