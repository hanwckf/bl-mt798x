#
# Copyright (c) 2021, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

MTK_PLAT		:=	plat/mediatek
MTK_PLAT_SOC		:=	${MTK_PLAT}/${PLAT}

include lib/libfdt/libfdt.mk
include lib/xz/xz.mk
include make_helpers/dep.mk

PLAT_INCLUDES		:=	-I${MTK_PLAT}/common/				\
				-I${MTK_PLAT}/common/drivers/uart		\
				-I${MTK_PLAT}/common/drivers/gpt		\
				-I${MTK_PLAT}/common/drivers/spi		\
				-I${MTK_PLAT}/common/drivers/efuse		\
				-I${MTK_PLAT}/common/drivers/efuse/include	\
				-Iinclude/plat/arm/common			\
				-Iinclude/plat/arm/common/aarch64		\
				-I${MTK_PLAT_SOC}/drivers/spmc/			\
				-I${MTK_PLAT_SOC}/drivers/timer/		\
				-I${MTK_PLAT_SOC}/drivers/gpio/			\
				-I${MTK_PLAT_SOC}/drivers/spi/			\
				-I${MTK_PLAT_SOC}/drivers/pll/			\
				-I${MTK_PLAT_SOC}/drivers/dram			\
				-I${MTK_PLAT_SOC}/drivers/devapc/		\
				-I${MTK_PLAT_SOC}/include/

PLAT_BL_COMMON_SOURCES	:=	lib/xlat_tables/xlat_tables_common.c		\
				lib/xlat_tables/aarch64/xlat_tables.c

DTS_NAME		:=	mt7986
FDT_SOURCES		+=	fdts/${DTS_NAME}.dts

BL2_SOURCES		:=	common/desc_image_load.c			\
				common/image_decompress.c		\
				drivers/delay_timer/delay_timer.c		\
				drivers/gpio/gpio.c				\
				drivers/io/io_storage.c				\
				drivers/io/io_block.c				\
				drivers/io/io_fip.c				\
				lib/cpus/aarch64/cortex_a53.S			\
				$(XZ_SOURCES)					\
				${MTK_PLAT}/common/drivers/uart/aarch64/hsuart.S\
				${MTK_PLAT}/common/mtk_plat_common.c		\
				${MTK_PLAT}/common/drivers/gpt/mt_gpt.c		\
				${MTK_PLAT}/common/drivers/spi/mtk_spi.c		\
				${MTK_PLAT}/common/drivers/efuse/mtk_efuse.c	\
				${MTK_PLAT_SOC}/aarch64/plat_helpers.S		\
				${MTK_PLAT_SOC}/aarch64/platform_common.c	\
				${MTK_PLAT_SOC}/dtb.S				\
				${MTK_PLAT_SOC}/bl2_plat_setup.c		\
				${MTK_PLAT_SOC}/drivers/timer/timer.c		\
				${MTK_PLAT_SOC}/drivers/gpio/mt7986_gpio.c	\
				${MTK_PLAT_SOC}/drivers/spi/boot_spi.c	\
				${MTK_PLAT_SOC}/drivers/pll/pll.c

CPPFLAGS		+=	-DDTB_PATH=\"${BUILD_PLAT}/fdts/${DTS_NAME}.dtb\"

# Whether supports DDR4
ifeq ($(DRAM_USE_DDR4), 1)
BL2_CPPFLAGS		+=	-DDRAM_USE_DDR4
AVAIL_DRAM_SIZE		:=	512 1024 2048
else
override DRAM_USE_DDR4	:=	0
AVAIL_DRAM_SIZE		:=	256 512
endif # END OF DRAM_USE_DDR4

# DDR4 frequency
ifeq ($(DDR4_FREQ_3200), 1)
BL2_CPPFLAGS		+=	-DDDR4_FREQ_3200
endif
ifeq ($(DDR4_FREQ_2666), 1)
BL2_CPPFLAGS		+=	-DDDR4_FREQ_2666
endif # END OF DDR4_FREQ_xxxx

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

HAVE_DRAM_OBJ_FILE	:= 	$(shell test -f ${MTK_PLAT_SOC}/drivers/dram/release/dram.o && echo yes)
ifeq ($(HAVE_DRAM_OBJ_FILE),yes)
PREBUILT_LIBS		+=	${MTK_PLAT_SOC}/drivers/dram/release/dram.o
BL2_SOURCES		+=	${MTK_PLAT_SOC}/drivers/dram/emicfg.c
else
PLAT_INCLUDES		+= 	-I${MTK_PLAT_SOC}/drivers/dram/inc/

BL2_SOURCES		+= \
	${MTK_PLAT_SOC}/drivers/dram/emicfg.c					\
	${MTK_PLAT_SOC}/drivers/dram/memory.c					\
	${MTK_PLAT_SOC}/drivers/dram/common/ANA_init_config.c \
	${MTK_PLAT_SOC}/drivers/dram/common/SRAM_init_lib.c \
	${MTK_PLAT_SOC}/drivers/dram/common/DIG_NONSHUF_config.c \
	${MTK_PLAT_SOC}/drivers/dram/common/DIG_SHUF_config.c \
	${MTK_PLAT_SOC}/drivers/dram/common/DRAM_config_collection.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_dv_freq_related.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_dv_main.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_dvfs.c \
	${MTK_PLAT_SOC}/drivers/dram/common/DRAMC_SUBSYS_config.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dvsim_dummy.c \
	${MTK_PLAT_SOC}/drivers/dram/common/Hal_io.c \
	${MTK_PLAT_SOC}/drivers/dram/common/sv_c_data_traffic.c \
	${MTK_PLAT_SOC}/drivers/dram/common/TX_RX_auto_gen_library.c \
	${MTK_PLAT_SOC}/drivers/dram/common/TX_path_auto_gen.c \
	${MTK_PLAT_SOC}/drivers/dram/common/RX_path_auto_gen.c \
	${MTK_PLAT_SOC}/drivers/dram/common/HW_FUNC_MANAGE.c \
	${MTK_PLAT_SOC}/drivers/dram/common/ESL_DPHY_FPGA_TX_settings.c \
	${MTK_PLAT_SOC}/drivers/dram/common/ESL_DPHY_FPGA_RX_settings.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_dv_api.c \
	${MTK_PLAT_SOC}/drivers/dram/common/emi.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_top.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_actiming.c \
	${MTK_PLAT_SOC}/drivers/dram/common/dramc_utility.c \
	${MTK_PLAT_SOC}/drivers/dram/common/MD32_initial.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_pi_main.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_pi_basic_api.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_pi_calibration_api.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_utility_pcddr.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_slt.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_tracking.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/IPM_actiming_setting_DDR3.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/IPM_actiming_setting_DDR4.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/DDR3_dram_init.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/DDR4_dram_init.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_debug.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_lowpower.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/dramc_pi_ddr_reserve.c \
	${MTK_PLAT_SOC}/drivers/dram/pcddr_cal/mtk_drm.c
#	${MTK_PLAT_SOC}/drivers/dram/fe/emi_fake_engine.c
endif # END OF HAVE_DRAM_OBJ_FILE

# IF BL33 is AARCH64, need to add this define
CPPFLAGS                +=      -DKERNEL_IS_DEFAULT_64BIT

# Include GICv3 driver files
include drivers/arm/gic/v3/gicv3.mk

BL31_SOURCES		+=	drivers/arm/cci/cci.c				\
				${GICV3_SOURCES}				\
				drivers/delay_timer/delay_timer.c		\
				drivers/delay_timer/generic_delay_timer.c	\
				lib/cpus/aarch64/aem_generic.S			\
				lib/cpus/aarch64/cortex_a53.S			\
				plat/common/plat_gicv3.c			\
				${MTK_PLAT}/common/drivers/uart/aarch64/hsuart.S\
				${MTK_PLAT}/common/mtk_plat_common.c		\
				${MTK_PLAT}/common/drivers/gpt/mt_gpt.c		\
				${MTK_PLAT}/common/drivers/spi/mtk_spi.c		\
				${MTK_PLAT}/common/drivers/efuse/mtk_efuse.c	\
				${MTK_PLAT}/common/mtk_sip_svc.c		\
				${MTK_PLAT_SOC}/aarch64/plat_helpers.S		\
				${MTK_PLAT_SOC}/aarch64/platform_common.c	\
				${MTK_PLAT_SOC}/bl31_plat_setup.c		\
				${MTK_PLAT_SOC}/plat_mt_gic.c			\
				${MTK_PLAT_SOC}/plat_pm.c			\
				${MTK_PLAT_SOC}/plat_sip_calls.c		\
				${MTK_PLAT_SOC}/drivers/spmc/mtspmc.c		\
				${MTK_PLAT_SOC}/drivers/timer/timer.c		\
				${MTK_PLAT_SOC}/drivers/spmc/mtspmc.c		\
				${MTK_PLAT_SOC}/drivers/gpio/mt7986_gpio.c	\
				${MTK_PLAT_SOC}/drivers/pll/pll.c		\
				${MTK_PLAT_SOC}/drivers/dram/emi_mpu.c		\
				${MTK_PLAT_SOC}/drivers/devapc/devapc.c		\
				${MTK_PLAT_SOC}/plat_topology.c

BL2_BASE		:=	0x201000
CPPFLAGS		+=	-DBL2_BASE=$(BL2_BASE)

# Enable workarounds for selected Cortex-A53 erratas.
ERRATA_A53_826319	:=	1
ERRATA_A53_836870	:=	1
ERRATA_A53_855873	:=	1

# indicate the reset vector address can be programmed
PROGRAMMABLE_RESET_ADDRESS	:=	1

$(eval $(call add_define,MTK_SIP_SET_AUTHORIZED_SECURE_REG_ENABLE))

# Do not enable SVE
ENABLE_SVE_FOR_NS		:=	0
MULTI_CONSOLE_API		:=	1

BL2_AT_EL3			:=	1

#
# Bromimage related build macros
#
DOIMAGEPATH		:=      tools/mediatek/bromimage
DOIMAGETOOL		:=      ${DOIMAGEPATH}/bromimage


#
# Boot device related build macros
#
ifndef BOOT_DEVICE
$(echo You must specify the boot device by provide BOOT_DEVICE= to \
	make parameter. Avaliable values: nor emmc sdmmc snand)
else # NO BOOT_DEVICE
ifeq ($(BOOT_DEVICE),ram)
BL2_SOURCES		+=	drivers/io/io_memmap.c			\
				${MTK_PLAT_SOC}/bl2_boot_ram.c
ifeq ($(RAM_BOOT_DEBUGGER_HOOK), 1)
CPPFLAGS		+=	-DRAM_BOOT_DEBUGGER_HOOK
endif
ifeq ($(RAM_BOOT_UART_DL), 1)
BL2_SOURCES		+=	${MTK_PLAT}/common/uart_dl.c
CPPFLAGS		+=	-DRAM_BOOT_UART_DL
endif
endif # END OF BOOT_DEVICE = ram
ifeq ($(BOOT_DEVICE),nor)
CPPFLAGS		+=	-DMTK_SPIM_NOR
BROM_HEADER_TYPE	:=	nor
BL2_SOURCES		+=	 drivers/mtd/nor/spi_nor.c			\
				 drivers/mtd/spi-mem/spi_mem.c			\
				 ${MTK_PLAT_SOC}/bl2_boot_spim_nor.c	\
				 ${MTK_PLAT}/common/mempool.c
PLAT_INCLUDES		+=	-Iinclude/lib/libfdt
CPPFLAGS		+=	-DMTK_MEM_POOL_BASE=0x40100000
endif # END OF BOOTDEVICE = nor
ifeq ($(BOOT_DEVICE),emmc)
BL2_SOURCES		+=	drivers/mmc/mmc.c				\
				drivers/partition/partition.c			\
				drivers/partition/gpt.c				\
				${MTK_PLAT}/common/drivers/mmc/mtk-sd.c		\
				${MTK_PLAT_SOC}/bl2_boot_mmc.c
BROM_HEADER_TYPE	:=	emmc
CPPFLAGS		+=	-DMSDC_INDEX=0
endif # END OF BOOTDEVICE = emmc
ifeq ($(BOOT_DEVICE),sdmmc)
BL2_SOURCES		+=	drivers/mmc/mmc.c				\
				drivers/partition/partition.c			\
				drivers/partition/gpt.c				\
				${MTK_PLAT}/common/drivers/mmc/mtk-sd.c		\
				${MTK_PLAT_SOC}/bl2_boot_mmc.c
BROM_HEADER_TYPE		:=	sdmmc
CPPFLAGS				+=  -DMSDC_INDEX=1
DEVICE_HEADER_OFFSET	?=  0x4400
endif # END OF BOOTDEVICE = sdmmc
ifeq ($(BOOT_DEVICE),snand)
include ${MTK_PLAT}/common/drivers/snfi/mtk-snand.mk
BROM_HEADER_TYPE	:=	snand
NAND_TYPE		?=	hsm:2k+64
BL2_SOURCES		+=	${MTK_SNAND_SOURCES}				\
				${MTK_PLAT_SOC}/bl2_boot_snand.c
PLAT_INCLUDES		+=	-I${MTK_PLAT}/common/drivers/snfi
CPPFLAGS		+=	-DMTK_MEM_POOL_BASE=0x40100000			\
				-DPRIVATE_MTK_SNAND_HEADER
endif # END OF BOOTDEVICE = snand (snfi)
ifeq ($(BOOT_DEVICE),spim-nand)
BROM_HEADER_TYPE	:=	spim-nand
NAND_TYPE		?=	spim:2k+64
BL2_SOURCES		+=	drivers/mtd/nand/core.c			\
				 drivers/mtd/nand/spi_nand.c			\
				 drivers/mtd/spi-mem/spi_mem.c			\
				 ${MTK_PLAT_SOC}/bl2_boot_spim_nand.c	\
				 ${MTK_PLAT}/common/mempool.c
PLAT_INCLUDES		+=	-Iinclude/lib/libfdt
CPPFLAGS		+=	-DMTK_MEM_POOL_BASE=0x40100000
endif # END OF BOOTDEVICE = spim-nand
ifeq ($(BROM_HEADER_TYPE),)
# $(error BOOT_DEVICE has invalid value. Please re-check.)
endif
endif # END OF BOOT_DEVICE

ifneq ($(filter snand spim-nand,$(BROM_HEADER_TYPE)),)
ifeq ($(NMBM),1)
include lib/nmbm/nmbm.mk
BL2_SOURCES		+=	${NMBM_SOURCES}
CPPFLAGS		+=	-DNMBM=1
ifneq ($(NMBM_MAX_RATIO),)
CPPFLAGS		+=	-DNMBM_MAX_RATIO=$(NMBM_MAX_RATIO)
endif
ifneq ($(NMBM_MAX_RESERVED_BLOCKS),)
CPPFLAGS		+=	-DNMBM_MAX_RESERVED_BLOCKS=$(NMBM_MAX_RESERVED_BLOCKS)
endif
ifneq ($(NMBM_DEFAULT_LOG_LEVEL),)
CPPFLAGS		+=	-DNMBM_DEFAULT_LOG_LEVEL=$(NMBM_DEFAULT_LOG_LEVEL)
endif
endif
endif # END OF SPI-NAND/NMBM

ifeq ($(MTK_BL2_DEBUG_CONSOLE), 1)
include ${MTK_PLAT_SOC}/bl2_debug_console/debug_console.mk
endif

# BL2PL for BL2 compression
ifeq ($(BL2_COMPRESS),1)
BL2PL_SOURCES		+=	${MTK_PLAT_SOC}/bl2pl/bl2pl_plat_setup.c	\
				${MTK_PLAT_SOC}/bl2pl/serial.c			\
				${MTK_PLAT_SOC}/bl2pl/timer.c			\
				${MTK_PLAT_SOC}/drivers/pll/pll.c

BL2PL_CPPFLAGS		+=	-DXZ_SIMPLE_PRINT_ERROR
BL2_CPPFLAGS		+=	-DUSING_BL2PL
endif # END OF BL2_COMPRESS

#
# Trusted board related build macros
#
ifneq (${TRUSTED_BOARD_BOOT},0)
include drivers/auth/mbedtls/mbedtls_crypto.mk
include drivers/auth/mbedtls/mbedtls_x509.mk
ifeq ($(MBEDTLS_DIR),)
$(error You must specify MBEDTLS_DIR when TRUSTED_BOARD_BOOT enabled)
endif
CPPFLAGS		+=	-DMTK_EFUSE_FIELD_NORMAL
AUTH_SOURCES		:=	drivers/auth/auth_mod.c			\
				drivers/auth/crypto_mod.c		\
				drivers/auth/img_parser_mod.c		\
				drivers/auth/tbbr/tbbr_cot_bl2.c	\
				drivers/auth/tbbr/tbbr_cot_common.c
BL2_SOURCES		+=	${AUTH_SOURCES}				\
				${MTK_PLAT_SOC}/mtk_tbbr.c		\
				${MTK_PLAT_SOC}/mtk_rotpk.S
ROT_KEY			:=	$(BUILD_PLAT)/rot_key.pem
ROTPK_HASH		:=	$(BUILD_PLAT)/rotpk_sha256.bin

$(eval $(call add_define_val,ROTPK_HASH,'"$(ROTPK_HASH)"'))
$(BUILD_PLAT)/bl1/mtk_rotpk.o: $(ROTPK_HASH)
$(BUILD_PLAT)/bl2/mtk_rotpk.o: $(ROTPK_HASH)

certificates: $(ROT_KEY)
$(ROT_KEY): | $(BUILD_PLAT)
	@echo "  OPENSSL $@"
	$(Q)openssl genrsa 2048 > $@ 2>/dev/null

$(ROTPK_HASH): $(ROT_KEY)
	@echo "  OPENSSL $@"
	$(Q)openssl rsa -in $< -pubout -outform DER 2>/dev/null |\
	openssl dgst -sha256 -binary > $@ 2>/dev/null
endif


#
# Read/Write efuse related build macros
#
HAVE_EFUSE_SRC_FILE	:= 	$(shell test -f ${MTK_PLAT}/common/drivers/efuse/src/efuse_cmd.c && echo yes)
ifeq ($(HAVE_EFUSE_SRC_FILE),yes)
PLAT_INCLUDES		+=	-I${MTK_PLAT}/common/drivers/efuse/src
BL31_SOURCES		+=	${MTK_PLAT}/common/drivers/efuse/src/efuse_cmd.c
else
PREBUILT_LIBS		+=	${MTK_PLAT_SOC}/drivers/efuse/release/efuse_cmd.o
endif

#
# Anti-rollback related build macros
#
DOVERSIONPATH           :=      tools/mediatek/ar-tool
DOVERSIONTOOL           :=      ${DOVERSIONPATH}/ar-tool
AUTO_AR_VER             :=      ${MTK_PLAT_SOC}/auto_ar_ver.c
AUTO_AR_CONF            :=      ${MTK_PLAT_SOC}/auto_ar_conf.mk
ifeq ($(ANTI_ROLLBACK),1)
ifneq ($(TRUSTED_BOARD_BOOT),1)
$(error You must enable TRUSTED_BOARD_BOOT when ANTI_ROLLBACK enabled)
endif
ifeq ($(ANTI_ROLLBACK_TABLE),)
$(error You must specify ANTI_ROLLBACK_TABLE when ANTI_ROLLBACK enabled)
endif

BL2_SOURCES             +=      ${AUTO_AR_VER}					\
				${MTK_PLAT}/common/mtk_ar.c
CPPFLAGS                +=      -DMTK_ANTI_ROLLBACK

ar_tool: $(DOVERSIONTOOL) $(AUTO_AR_VER) $(AUTO_AR_CONF)
	$(eval $(shell sed -n 1p $(AUTO_AR_CONF)))
	$(eval $(call CERT_REMOVE_CMD_OPT,0,--tfw-nvctr))
	$(eval $(call CERT_REMOVE_CMD_OPT,0,--ntfw-nvctr))
	$(eval $(call CERT_ADD_CMD_OPT,$(BL_AR_VER),--tfw-nvctr))
	$(eval $(call CERT_ADD_CMD_OPT,$(BL_AR_VER),--ntfw-nvctr))
	@echo "BL_AR_VER     = $(BL_AR_VER)"

$(AUTO_AR_VER): $(DOVERSIONTOOL)
	$(Q)$(DOVERSIONTOOL) bl_ar_table create_ar_ver $(ANTI_ROLLBACK_TABLE) $(AUTO_AR_VER)

$(AUTO_AR_CONF): $(DOVERSIONTOOL)
	$(Q)$(DOVERSIONTOOL) bl_ar_table create_ar_conf $(ANTI_ROLLBACK_TABLE) $(AUTO_AR_CONF)

$(DOVERSIONTOOL):
	$(Q)$(MAKE) --no-print-directory -C $(DOVERSIONPATH)
else
ar_tool:
	@echo "Warning: anti-rollback function has been turn-off"
endif

# Make sure make command parameter reflects to .o files immediately
$(call GEN_DEP_RULES,bl2,emicfg bl2_boot_ram bl2_boot_snand bl2_boot_spim_nand mtk_efuse bl2_plat_setup)
$(call GEN_DEP_RULES,bl31,mtk_efuse)

bl2: bl2_update_dep

bl2_update_dep:
	${Q}$(call CHECK_DEP,bl2,emicfg,DRAM_USE_DDR4 DRAM_SIZE_LIMIT DRAM_DEBUG_LOG)
	${Q}$(call CHECK_DEP,bl2,bl2_plat_setup,BOOT_DEVICE BL2_COMPRESS)
	${Q}$(call CHECK_DEP,bl2,bl2_boot_ram,RAM_BOOT_DEBUGGER_HOOK RAM_BOOT_UART_DL)
	${Q}$(call CHECK_DEP,bl2,bl2_boot_snand,NMBM NMBM_MAX_RATIO NMBM_MAX_RESERVED_BLOCKS NMBM_DEFAULT_LOG_LEVEL)
	${Q}$(call CHECK_DEP,bl2,bl2_boot_spim_nand,NMBM NMBM_MAX_RATIO NMBM_MAX_RESERVED_BLOCKS NMBM_DEFAULT_LOG_LEVEL)
	${Q}$(call CHECK_DEP,bl2,mtk_efuse,ANTI_ROLLBACK TRUSTED_BOARD_BOOT)

bl31: bl31_update_dep

bl31_update_dep:
	${Q}$(call CHECK_DEP,bl31,mtk_efuse,ANTI_ROLLBACK TRUSTED_BOARD_BOOT)

# BL2 compress
ifeq ($(BL2_COMPRESS),1)
BL2PLIMAGEPATH		:= tools/bl2plimage
BL2PLIMAGETOOL		:= ${BL2PLIMAGEPATH}/bl2plimage

$(BL2PLIMAGETOOL):
	$(Q)$(MAKE) --no-print-directory -C $(BL2PLIMAGEPATH)

$(BUILD_PLAT)/bl2-sfx.bin: $(BUILD_PLAT)/bl2pl.bin $(BUILD_PLAT)/bl2.bin.xz.plimg
	$(Q)cat $^ > $@

$(BUILD_PLAT)/bl2.bin.xz.plimg: $(BUILD_PLAT)/bl2.bin.xz $(BL2PLIMAGETOOL)
	$(Q)$(BL2PLIMAGETOOL) -a $(BL2_BASE) $(BUILD_PLAT)/bl2.bin.xz $@

$(BUILD_PLAT)/bl2.bin.xz: $(BUILD_PLAT)/bl2.bin
	$(ECHO) "  XZ      $@"
	$(Q)xz -e -k -9 -C crc32 $< --stdout > $@

.PHONY: $(BUILD_PLAT)/bl2.xz.plimg

BL2_IMG_PAYLOAD := $(BUILD_PLAT)/bl2-sfx.bin
else
BL2_IMG_PAYLOAD := $(BUILD_PLAT)/bl2.bin
endif # END OF BL2_COMPRESS

# FIP compress
ifeq ($(FIP_COMPRESS),1)
BL31_PRE_TOOL_FILTER	:= XZ
BL32_PRE_TOOL_FILTER	:= XZ
BL33_PRE_TOOL_FILTER	:= XZ
endif

# Build dtb before embedding to BL2
${BUILD_PLAT}/bl2/dtb.o: ${BUILD_PLAT}/fdts/${DTS_NAME}.dtb

ifeq ($(BOOT_DEVICE),ram)
bl2: $(BL2_IMG_PAYLOAD)
else
bl2: $(BUILD_PLAT)/bl2.img
endif

ifneq ($(BROM_SIGN_KEY),)
$(BUILD_PLAT)/bl2.img: $(BROM_SIGN_KEY)
endif

MTK_SIP_KERNEL_BOOT_ENABLE := 1
$(eval $(call add_define,MTK_SIP_KERNEL_BOOT_ENABLE))

$(BUILD_PLAT)/bl2.img: $(BL2_IMG_PAYLOAD) $(DOIMAGETOOL)
	-$(Q)rm -f $@.signkeyhash
	$(Q)$(DOIMAGETOOL) -c mt7986 -f $(BROM_HEADER_TYPE) -a $(BL2_BASE) -d -e	\
		$(if $(BROM_SIGN_KEY), -s sha256+rsa-pss -k $(BROM_SIGN_KEY))	\
		$(if $(BROM_SIGN_KEY), -p $@.signkeyhash)			\
		$(if $(DEVICE_HEADER_OFFSET), -o $(DEVICE_HEADER_OFFSET))	\
		$(if $(BL_AR_VER), -r $(BL_AR_VER))				\
		$(if $(NAND_TYPE), -n $(NAND_TYPE))				\
		$(BL2_IMG_PAYLOAD) $@

$(DOIMAGETOOL):
	$(Q)$(MAKE) --no-print-directory -C $(DOIMAGEPATH)

.PHONY: $(BUILD_PLAT)/bl2.img $(AUTO_AR_VER) $(AUTO_AR_CONF)
