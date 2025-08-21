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
include ${MAKE_HELPERS_DIRECTORY}common.mk

PROJECT := ar-table$(.exe)

.PHONY: all clean distclean

all: ${PROJECT}

%: %.py Makefile
	$(s)echo "  CP $< $@"
	$(q)cp $< $@
	$(s)echo
	$(s)echo "Built $@ successfully"
	$(s)echo

clean:
	$(q)rm -rf ${PROJECT}

distclean: clean
