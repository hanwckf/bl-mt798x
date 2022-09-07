# SPDX-License-Identifier: GPL-2.0

ifeq ($(CONFIG_SPL_BUILD),)

obj-y += fs.o failsafe.o

fs.o: $(shell export LC_ALL=C;cd $(srctree)/failsafe;rm -rf httpd-fsdata.c;./makefsdata)

CLEAN_FILES += httpd-fsdata.c

endif
