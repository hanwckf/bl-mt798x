#
# Copyright (c) 2025, MediaTek Inc. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Name of the platform defined source file name,
# which contains platform defined UUID entries populated
# in the plat_def_toc_entries[].
PLAT_DEF_UUID_FILE_NAME	:= plat_def_uuid_config

INCLUDE_PATHS		+= -I../../plat/mediatek/apsoc_common/bl2/include -I./

PLAT_DEF_UUID		:= yes

ifeq (${PLAT_DEF_UUID},yes)
HOSTCCFLAGS += -DPLAT_DEF_FIP_UUID -DMTK_FIP_CHKSUM

PLAT_OBJECTS += plat_fiptool/mediatek/${PLAT_DEF_UUID_FILE_NAME}.o	\
	plat_fiptool/mediatek/sha256.o	\
	plat_fiptool/mediatek/crc32.o
endif

OBJECTS += ${PLAT_OBJECTS}
