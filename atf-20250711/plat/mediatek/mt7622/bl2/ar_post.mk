#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Anti-rollback related build macros
#
DOVERSIONPATH		:=	tools/mediatek/ar-table
DOVERSIONTOOL		:=	$(DOVERSIONPATH)/ar-table

ifeq ($(ANTI_ROLLBACK),1)

ar_table: $(DOVERSIONTOOL) $(AUTO_AR_TABLE) $(AUTO_AR_CONF)
	$(eval $(shell sed -n 1p $(AUTO_AR_CONF)))
	$(eval $(shell sed -n 2p $(AUTO_AR_CONF)))
	$(eval $(shell sed -n 3p $(AUTO_AR_CONF)))
	$(eval $(call CERT_REMOVE_CMD_OPT,0,--tfw-nvctr))
	$(eval $(call CERT_REMOVE_CMD_OPT,0,--ntfw-nvctr))
	$(eval $(call CERT_ADD_CMD_OPT,$(TFW_NVCTR_VAL),--tfw-nvctr))
	$(eval $(call CERT_ADD_CMD_OPT,$(NTFW_NVCTR_VAL),--ntfw-nvctr))
	@echo "TFW_NVCTR_VAL  = $(TFW_NVCTR_VAL)"
	@echo "NTFW_NVCTR_VAL = $(NTFW_NVCTR_VAL)"
	@echo "BL_AR_VER     = $(BL_AR_VER)"

$(AUTO_AR_TABLE): $(DOVERSIONTOOL)
	$(q)$(DOVERSIONTOOL) create_ar_table $(AR_TABLE_XML) $(AUTO_AR_TABLE)

$(AUTO_AR_CONF): $(DOVERSIONTOOL)
	$(q)$(DOVERSIONTOOL) create_ar_conf $(AR_TABLE_XML) $(AUTO_AR_CONF)

$(DOVERSIONTOOL):
	$(q)$(MAKE) --no-print-directory -C $(DOVERSIONPATH)
else
ar_table:
	@echo "Warning: anti-rollback function has been turned off"
endif

.PHONY: $(AUTO_AR_TABLE) $(AUTO_AR_CONF)
