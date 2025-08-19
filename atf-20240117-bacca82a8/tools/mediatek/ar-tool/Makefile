#
# Copyright (C) 2019 MediaTek Inc.
#
# Author: Sam Shih <sam.shih@mediatek.com>
#
# SPDX-License-Identifier:     BSD-3-Clause
# https://spdx.org/licenses
#

MAKE_HELPERS_DIRECTORY := ../../../make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}build_env.mk

PROJECT := ar-tool${BIN_EXT}

.PHONY: all clean distclean

V ?= 0

ifeq (${V},0)
  Q := @
else
  Q :=
endif

all: ${PROJECT}

%: %.py Makefile
	@echo "  CP $< $@"
	${Q}cp $< $@
	@${ECHO_BLANK_LINE}
	@echo "Built $@ successfully"
	@${ECHO_BLANK_LINE}

clean:
	${Q}$(call SHELL_DELETE_ALL, ${PROJECT})

distclean: clean
