#
# Copyright (C) 2020 MediaTek Inc. All rights reserved.
#
# Author: Weijie Gao <weijie.gao@mediatek.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

MAKE_HELPERS_DIRECTORY := ../../make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}build_env.mk

PROJECT := bl2plimage${BIN_EXT}
OBJECTS := bl2plimage.o
V ?= 0

override CPPFLAGS += -D_GNU_SOURCE
HOSTCCFLAGS := -Wall -Werror -pedantic -std=c11

ifeq (${DEBUG},1)
  HOSTCCFLAGS += -ggdb -O0 -DDEBUG
else
  HOSTCCFLAGS += -O2
endif

INCLUDE_PATHS := -I../../include/bl2pl

ifeq (${V},0)
  Q := @
else
  Q :=
endif

HOSTCC ?= gcc

.PHONY: all clean distclean

all: ${PROJECT}

${PROJECT}: ${OBJECTS} Makefile
	@echo "  HOSTLD  $@"
	${Q}${HOSTCC} ${OBJECTS} -o $@ ${LDLIBS}
	@${ECHO_BLANK_LINE}
	@echo "Built $@ successfully"
	@${ECHO_BLANK_LINE}

%.o: %.c Makefile
	@echo "  HOSTCC  $<"
	${Q}${HOSTCC} -c ${CPPFLAGS} ${HOSTCCFLAGS} ${INCLUDE_PATHS} $< -o $@

clean:
	$(call SHELL_DELETE_ALL, ${PROJECT} ${OBJECTS})

distclean: clean
