# SPDX-License-Identifier: BSD-3-Clause
#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
# Auther: Weijie Gao <weijie.gao@mediatek.com>
#
# Script for Kconfig, post-configuration
#

KCONFIG_DIR := $(CURDIR)/Kconfiglib
MENUCONFIG := $(KCONFIG_DIR)/menuconfig.py
SAVEDEFCONFIG := $(KCONFIG_DIR)/savedefconfig.py
OLDCONFIG := $(KCONFIG_DIR)/oldconfig.py
DEFCONFIG := $(KCONFIG_DIR)/defconfig.py

TOP_CONFIG := $(CURDIR)/Config.in

PYTHON := $(shell which python)

check_python:
ifeq ($(PYTHON),)
	@echo ">> Unable to find python"
	@echo ">> You must have python installed in order"
	@echo ">> to use 'make menuconfig' and related commands"
	@exit 1;
endif

.PHONY: check_python

defconfig: check_python
	$(Q)mkdir -p $(BUILD_BASE) && ( \
		export CONFIG_=; \
		export srctree=$(CURDIR); \
		cd $(BUILD_BASE); \
		$(DEFCONFIG) ".config" --kconfig $(TOP_CONFIG) \
	)

menuconfig: check_python
	$(Q)mkdir -p $(BUILD_BASE) && ( \
		export CONFIG_=; \
		export srctree=$(CURDIR); \
		cd $(BUILD_BASE); \
		$(MENUCONFIG) $(TOP_CONFIG) \
	)

savedefconfig: check_python
	$(Q)mkdir -p $(BUILD_BASE) && ( \
		export CONFIG_=; \
		export srctree=$(CURDIR); \
		cd $(BUILD_BASE); \
		$(SAVEDEFCONFIG) --kconfig $(TOP_CONFIG) \
				 --out defconfig \
	)

%_defconfig: check_python
	$(Q)mkdir -p $(BUILD_BASE) && ( \
		export CONFIG_=; \
		export srctree=$(CURDIR); \
		[ -f "configs/$@" ] && { \
			cd $(BUILD_BASE); \
			$(DEFCONFIG) "$(CURDIR)/configs/$@" \
				     --kconfig $(TOP_CONFIG); \
			$(OLDCONFIG) $(TOP_CONFIG); \
		} || exit 1 \
	)

%_defconfig_update: check_python
	$(Q)mkdir -p $(BUILD_BASE) && ( \
		export CONFIG_=; \
		export srctree=$(CURDIR); \
		cd $(BUILD_BASE); \
		$(SAVEDEFCONFIG) --kconfig $(TOP_CONFIG) \
				 --out "$(CURDIR)/configs/$(patsubst %_update,%,$@)" \
	)

%_defconfig_refresh: check_python
	$(Q)mkdir -p $(BUILD_BASE) && ( \
		export CONFIG_=; \
		export srctree=$(CURDIR); \
		[ -f "configs/$(patsubst %_refresh,%,$@)" ] && { \
			cd $(BUILD_BASE); \
			$(DEFCONFIG) "$(CURDIR)/configs/$(patsubst %_refresh,%,$@)" \
				     --kconfig $(TOP_CONFIG); \
			$(OLDCONFIG) $(TOP_CONFIG); \
			$(SAVEDEFCONFIG) --kconfig $(TOP_CONFIG) \
					 --out "$(CURDIR)/configs/$(patsubst %_refresh,%,$@)"; \
		} || exit 1 \
	)

# Extra rules
ifeq ($(_BUILD_FIP),y)
all: fip
endif

ifeq ($(_ENABLE_AR),y)
ifeq ($(_SUPPORTS_AR_V1),y)
all: ar_table
endif
ifeq ($(_SUPPORTS_AR_V2),y)
all: ar_tool
endif
endif
