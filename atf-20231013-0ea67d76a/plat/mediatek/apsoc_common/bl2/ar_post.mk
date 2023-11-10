#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Anti-rollback related build macros
#
DOVERSIONPATH		:=	tools/mediatek/ar-tool
DOVERSIONTOOL		:=	$(DOVERSIONPATH)/ar-tool

ifeq ($(ANTI_ROLLBACK),1)

AUTO_AR_CONF		:=	$(MTK_PLAT_SOC)/auto_ar_conf.mk

ar_tool: $(DOVERSIONTOOL) $(AUTO_AR_VER) $(AUTO_AR_CONF)
	$(eval $(shell sed -n 1p $(AUTO_AR_CONF)))
	$(eval $(call CERT_REMOVE_CMD_OPT,0,--tfw-nvctr))
	$(eval $(call CERT_REMOVE_CMD_OPT,0,--ntfw-nvctr))
	$(eval $(call CERT_ADD_CMD_OPT,$(BL_AR_VER),--tfw-nvctr))
	$(eval $(call CERT_ADD_CMD_OPT,$(BL_AR_VER),--ntfw-nvctr))
	@echo "BL_AR_VER = $(BL_AR_VER)"

$(AUTO_AR_VER): $(DOVERSIONTOOL) $(ANTI_ROLLBACK_TABLE)
	$(Q)$(DOVERSIONTOOL) bl_ar_table create_ar_ver $(ANTI_ROLLBACK_TABLE) $(AUTO_AR_VER)

$(AUTO_AR_CONF): $(DOVERSIONTOOL) $(ANTI_ROLLBACK_TABLE)
	$(Q)$(DOVERSIONTOOL) bl_ar_table create_ar_conf $(ANTI_ROLLBACK_TABLE) $(AUTO_AR_CONF)

$(DOVERSIONTOOL):
	$(Q)$(MAKE) --no-print-directory -C $(DOVERSIONPATH)
else
ar_tool:
	@echo "Warning: anti-rollback function has been turned off"
endif

.PHONY: $(AUTO_AR_VER) $(AUTO_AR_CONF)
