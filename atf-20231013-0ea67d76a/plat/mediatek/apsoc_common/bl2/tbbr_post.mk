#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Signoffline related build macros
#
SIGNOFFLINEPATH		:=	tools/mediatek/signoffline
SIGNOFFLINETOOL		:=	$(SIGNOFFLINEPATH)/signoffline

#
# Trusted board boot related build macros
#
ifeq ($(TRUSTED_BOARD_BOOT),1)

ROT_KEY			:=	$(BUILD_PLAT)/rot_key.pem

$(BUILD_PLAT)/bl2/mtk_rotpk.o: $(ROTPK_HASH)

certificates: $(ROT_KEY)
$(ROT_KEY): | $(BUILD_PLAT)
	@echo "  OPENSSL $@"
	$(Q)openssl genrsa 2048 > $@ 2>/dev/null

ifneq ($(ROT_PUB_KEY),)
$(ROTPK_HASH): $(ROT_PUB_KEY) $(SIGNOFFLINETOOL)
	@echo "  OPENSSL $@"
	$(Q)openssl rsa -in $< -pubin -outform DER 2>/dev/null | \
		openssl dgst -sha256 -binary > $@ 2>/dev/null

$(SIGNOFFLINETOOL): FORCE
	$(Q)$(MAKE) --no-print-directory -C $(SIGNOFFLINEPATH)

else
$(ROTPK_HASH): $(ROT_KEY)
	@echo "  OPENSSL $@"
	$(Q)openssl rsa -in $< -pubout -outform DER 2>/dev/null | \
		openssl dgst -sha256 -binary > $@ 2>/dev/null
endif # End of ROT_PUB_KEY

endif
