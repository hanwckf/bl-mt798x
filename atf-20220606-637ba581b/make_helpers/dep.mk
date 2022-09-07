#
# Copyright (C) 2021, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Create a dependency rule for a .o file
# Arguments:
#   $(1) = image_type (scp_bl2, bl33, etc.)
#   $(2) = source file name (without suffix)
define ADD_DEP_RULE
$(BUILD_PLAT)/$(1)/$(2).o: $(BUILD_PLAT)/$(1)/$(2).cfg.h
endef

# Create dependency rules for .o files
# Arguments:
#   $(1) = image_type (scp_bl2, bl33, etc.)
#   $(2) = source file names (without suffix)
define GEN_DEP_RULES
$(foreach f,$(2),$(eval $(call ADD_DEP_RULE,$(1),$(f))))
endef

# Generate the content of a dependency file
# Arguments:
#   $(1) = list of dependency variable names
define GEN_DEP_HEADER_TEXT
$(foreach dep,$(1),/* #define $(dep) $($(dep)) */\n)
endef

# Create commands to check & update a dependency file for a .o file
# Arguments:
#   $(1) = image_type (scp_bl2, bl33, etc.)
#   $(2) = dependency header file path
#   $(3) = list of dependency variable names
define CHECK_DEP
	mkdir -p $(BUILD_PLAT)/$(1); \
	echo "$(call GEN_DEP_HEADER_TEXT,$(3))" > $(BUILD_PLAT)/$(1)/$(2).cfg-new.h; \
	if ! test -f $(BUILD_PLAT)/$(1)/$(2).cfg.h; then \
		mv $(BUILD_PLAT)/$(1)/$(2).cfg-new.h $(BUILD_PLAT)/$(1)/$(2).cfg.h; \
	else \
		if ! cmp -s $(BUILD_PLAT)/$(1)/$(2).cfg.h $(BUILD_PLAT)/$(1)/$(2).cfg-new.h; then \
			mv -f $(BUILD_PLAT)/$(1)/$(2).cfg-new.h $(BUILD_PLAT)/$(1)/$(2).cfg.h; \
		else \
			rm -f $(BUILD_PLAT)/$(1)/$(2).cfg-new.h; \
		fi \
	fi
endef
