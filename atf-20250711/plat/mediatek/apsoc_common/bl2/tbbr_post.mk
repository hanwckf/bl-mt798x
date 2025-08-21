#
# Copyright (c) 2023, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

#
# Trusted board boot related build macros
#
ifeq ($(TRUSTED_BOARD_BOOT),1)

ROT_KEY			?=	$(BUILD_PLAT)/rot_key.pem

$(BUILD_PLAT)/bl2/mtk_rotpk.o: $(ROTPK_HASH)

certificates: $(ROT_KEY)
$(ROT_KEY): | $(BUILD_PLAT)
	@echo "  OPENSSL $@"
	$(q)openssl genrsa 2048 > $@ 2>/dev/null

ifneq ($(OFFLINE_SIGN),)
CRT_ARGS		+=	-o
CREATE_KEYS		:=	0

ROT_KEY			?=	$(BUILD_PLAT)/rot_key.pem
SCP_BL2_KEY		?=	$(BUILD_PLAT)/scp_fw_key.pem
TRUSTED_WORLD_KEY	?=	$(BUILD_PLAT)/trusted_key.pem
NON_TRUSTED_WORLD_KEY	?=	$(BUILD_PLAT)/non_trusted_key.pem
BL31_KEY		?=	$(BUILD_PLAT)/soc_fw_key.pem
BL32_KEY		?=	$(BUILD_PLAT)/tos_fw_key.pem
BL33_KEY		?=	$(BUILD_PLAT)/nt_fw_key.pem

$(if ${SCP_BL2_KEY},$(eval $(call CERT_ADD_CMD_OPT,${SCP_BL2_KEY},--scp-fw-key)))
$(if ${BL32_KEY},$(eval $(call CERT_ADD_CMD_OPT,${BL32_KEY},--tos-fw-key)))

$(ROTPK_HASH): $(ROT_KEY)
	@echo "  OPENSSL $@"
	$(q)openssl rsa -in $< -pubin -outform DER 2>/dev/null | \
		openssl dgst -$(HASH_ALG) -binary > $@ 2>/dev/null
else
$(ROTPK_HASH): $(ROT_KEY)
	@echo "  OPENSSL $@"
	$(q)openssl rsa -in $< -pubout -outform DER 2>/dev/null | \
		openssl dgst -$(HASH_ALG) -binary > $@ 2>/dev/null
endif # End of OFFLINE_SIGN

endif
