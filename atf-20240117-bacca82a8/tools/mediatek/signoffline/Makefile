#
# Copyright (C) 2019 MediaTek Inc.
#
# SPDX-License-Identifier:     BSD-3-Clause
# https://spdx.org/licenses
#

MAKE_HELPERS_DIRECTORY := ../../../make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}build_env.mk

PROJECT := signoffline${BIN_EXT}
OBJECTS := signoffline.o
V ?= 0

override CPPFLAGS += -D_GNU_SOURCE
HOSTCCFLAGS := -Wall -Wno-deprecated-declarations -Werror -pedantic -std=c11 -ggdb

ifeq (${DEBUG},1)
  HOSTCCFLAGS += -O0 -DDEBUG
else
  HOSTCCFLAGS += -O2
endif

LDLIBS := -lcrypto

ifeq (${V},0)
  Q := @
else
  Q :=
endif

HOSTCC ?= gcc

DEPS := $(patsubst %.o,%.d,$(OBJECTS))

.PHONY: all clean distclean

all: ${PROJECT}

${PROJECT}: ${OBJECTS} Makefile
	@echo "  HOSTLD  $@"
	${Q}${HOSTCC} ${OBJECTS} -o $@ ${LDLIBS}
	@${ECHO_BLANK_LINE}
	@echo "Built $@ successfully"
	@${ECHO_BLANK_LINE}

%.o: %.c %.h Makefile
	@echo "  HOSTCC  $<"
	${Q}${HOSTCC} -c ${CPPFLAGS} ${HOSTCCFLAGS} -MD -MP $< -o $@

-include $(DEPS)

clean:
	$(call SHELL_DELETE_ALL, ${PROJECT} ${OBJECTS})

distclean: clean
