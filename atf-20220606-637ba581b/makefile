TOPDIR := $(CURDIR)
KCONFIG := $(TOPDIR)/config.in
MENUCONFIG := $(TOPDIR)/Kconfiglib/menuconfig.py
SAVEDEFCONFIG := $(TOPDIR)/Kconfiglib/savedefconfig.py
OLDCONFIG := $(TOPDIR)/Kconfiglib/oldconfig.py
DEFCONFIG := $(TOPDIR)/Kconfiglib/defconfig.py

ifneq ("$(wildcard $(TOPDIR)/.config)", "")
include $(TOPDIR)/.config
MTK_CONFIG_EXIST := y
MAKE_ARGS := PLAT=$(CONFIG_PLAT)
TARGET_ARGS :=
MAKE_ARGS += CROSS_COMPILE=$(CONFIG_CROSS_COMPILER)
ifeq (${CONFIG_FPGA},y)
FPGA = 1
MAKE_ARGS += FPGA=$(FPGA)
endif
ifeq (${CONFIG_NEED_SBC},y)
MAKE_ARGS += TRUSTED_BOARD_BOOT=1
MAKE_ARGS += GENERATE_COT=1
MAKE_ARGS += ROT_KEY=$(CONFIG_ROT_KEY)
MAKE_ARGS += BROM_SIGN_KEY=$(CONFIG_BROM_SIGN_KEY)
endif
ifeq (${CONFIG_NEED_MBEDTLS},y)
MAKE_ARGS += MBEDTLS_DIR=$(CONFIG_MBEDTLS_DIR)
endif
MAKE_ARGS += BOOT_DEVICE=$(CONFIG_BOOT_DEVICE)
ifeq (${CONFIG_FLASH_DEVICE_SNFI_SNAND},y)
MAKE_ARGS += NAND_TYPE=$(CONFIG_SNFI_SNAND_TYPE)
endif
ifeq (${CONFIG_NMBM},y)
NMBM := 1
MAKE_ARGS += NMBM=$(NMBM)
endif
ifeq (${CONFIG_FLASH_DEVICE_SPIM_NAND},y)
MAKE_ARGS += NAND_TYPE=$(CONFIG_SPIM_NAND_TYPE)
endif
ifeq (${CONFIG_DRAM_DDR3},y)
MAKE_ARGS += DRAM_USE_DDR4=0
endif
ifeq (${CONFIG_DRAM_DDR3_FLYBY},y)
MAKE_ARGS += DRAM_USE_DDR4=0
MAKE_ARGS += DDR3_FLYBY=1
endif
ifeq (${CONFIG_DRAM_DDR4},y)
MAKE_ARGS += DRAM_USE_DDR4=1
endif
ifeq (${CONFIG_DDR3_FREQ_2133},y)
MAKE_ARGS += DDR3_FREQ_2133=1
endif
ifeq (${CONFIG_DDR3_FREQ_1866},y)
MAKE_ARGS += DDR3_FREQ_1866=1
endif
ifeq (${CONFIG_DDR4_FREQ_2666},y)
MAKE_ARGS += DDR4_FREQ_2666=1
endif
ifeq (${CONFIG_DDR4_FREQ_3200},y)
MAKE_ARGS += DDR4_FREQ_3200=1
endif
ifeq (${CONFIG_QFN},y)
MAKE_ARGS += BOARD_QFN=1
endif
ifeq (${CONFIG_BGA},y)
MAKE_ARGS += BOARD_BGA=1
endif
ifeq (${CONFIG_DRAM_SIZE_AUTO},y)
endif
ifeq (${CONFIG_DRAM_SIZE_256},y)
MAKE_ARGS += DRAM_SIZE_LIMIT=256
endif
ifeq (${CONFIG_DRAM_SIZE_512},y)
MAKE_ARGS += DRAM_SIZE_LIMIT=512
endif
ifeq (${CONFIG_DRAM_SIZE_1024},y)
MAKE_ARGS += DRAM_SIZE_LIMIT=1024
endif
ifeq (${CONFIG_DRAM_SIZE_2048},y)
MAKE_ARGS += DRAM_SIZE_LIMIT=2048
endif
ifeq (${CONFIG_LOG_LEVEL_NONE},y)
MAKE_ARGS += LOG_LEVEL=0
endif
ifeq (${CONFIG_LOG_LEVEL_ERROR},y)
MAKE_ARGS += LOG_LEVEL=10
endif
ifeq (${CONFIG_LOG_LEVEL_NOTICE},y)
MAKE_ARGS += LOG_LEVEL=20
endif
ifeq (${CONFIG_LOG_LEVEL_WARNING},y)
MAKE_ARGS += LOG_LEVEL=30
endif
ifeq (${CONFIG_LOG_LEVEL_INFO},y)
MAKE_ARGS += LOG_LEVEL=40
endif
ifeq (${CONFIG_LOG_LEVEL_VERBOSE},y)
MAKE_ARGS += LOG_LEVEL=50
endif
ifeq (${CONFIG_DRAM_DEBUG_LOG},y)
MAKE_ARGS += DRAM_DEBUG_LOG=1
endif

ifeq ($(CONFIG_NEED_BL33),y)
MAKE_ARGS += BL33=$(CONFIG_BL33)
endif
ifeq ($(CONFIG_AARCH32),y)
MAKE_ARGS += ARCH=aarch32
endif
ifeq ($(CONFIG_NEED_FIP),y)
TARGET_ARGS += all fip
else
ifeq ($(CONFIG_TARGET_BL2),y)
TARGET_ARGS += bl2
endif
ifeq ($(CONFIG_TARGET_BL31),y)
TARGET_ARGS += bl31
endif
endif
endif

PYTHON := $(shell which python)

ifeq (${MTK_CONFIG_EXIST},y)

all: atf

atf: clean
	make -f $(TOPDIR)/Makefile $(MAKE_ARGS) $(TARGET_ARGS)

clean:
	make -f $(TOPDIR)/Makefile $(MAKE_ARGS) clean

else

# ignore %_defconfig, defconfig and menuconfig to prevent re-define / overwrite
$(filter-out %config, $(MAKECMDGOALS)):
	make -f $(TOPDIR)/Makefile ${MAKEOVERRIDES} $(MAKECMDGOALS)

endif

check_python:
ifeq ($(PYTHON),)
	@echo ">> Unable to find python"
	@echo ">> You must have python installed in order"
	@echo ">> to use 'make menuconfig'"
	@exit 1;
endif

%_defconfig: check_python
	$(PYTHON) $(DEFCONFIG) $(TOPDIR)/configs/$@ --kconfig $(KCONFIG)
	$(PYTHON) $(OLDCONFIG) $(KCONFIG)

defconfig:
	$(PYTHON) $(DEFCONFIG) --kconfig $(KCONFIG)

menuconfig: check_python
	$(PYTHON) $(MENUCONFIG) $(KCONFIG)

savedefconfig: check_python
	$(PYTHON) $(SAVEDEFCONFIG) --kconfig $(KCONFIG) \
				   --out $(TOPDIR)/defconfig

%_defconfig_update: check_python savedefconfig
	cp $(TOPDIR)/defconfig $(TOPDIR)/configs/$(subst _update,,$@)
