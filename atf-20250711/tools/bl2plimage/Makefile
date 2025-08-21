#
# Copyright (C) 2020 MediaTek Inc. All rights reserved.
#
# Author: Weijie Gao <weijie.gao@mediatek.com>
#
# SPDX-License-Identifier: BSD-3-Clause
#

MAKE_HELPERS_DIRECTORY := ../../make_helpers/
include ${MAKE_HELPERS_DIRECTORY}build_macros.mk
include ${MAKE_HELPERS_DIRECTORY}common.mk

PROJECT := bl2plimage$(.exe)
OBJECTS := bl2plimage.o

HOSTCCFLAGS := -Wall -Werror -pedantic -std=c11 -D_GNU_SOURCE

ifeq (${DEBUG},1)
  HOSTCCFLAGS += -ggdb -O0 -DDEBUG
else
  HOSTCCFLAGS += -O2
endif

INCLUDE_PATHS := -I../../include/bl2pl

HOSTCC ?= gcc

.PHONY: all clean distclean

all: ${PROJECT}

${PROJECT}: ${OBJECTS} Makefile
	$(s)echo "  HOSTLD  $@"
	$(q)${HOSTCC} ${OBJECTS} -o $@ ${LDLIBS}
	$(s)echo
	$(s)echo "Built $@ successfully"
	$(s)echo

%.o: %.c Makefile
	$(s)echo "  HOSTCC  $<"
	$(q)${HOSTCC} -c ${HOSTCCFLAGS} ${INCLUDE_PATHS} $< -o $@

clean:
	$(q)rm -rf ${PROJECT} ${OBJECTS}

distclean: clean
