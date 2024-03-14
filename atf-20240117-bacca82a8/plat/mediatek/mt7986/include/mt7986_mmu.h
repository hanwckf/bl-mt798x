#ifndef MT7986_MMU_H
#define MT7986_MMU_H

#include <platform_def.h>

#define MAP_SEC_SYSRAM	MAP_REGION_FLAT(TZRAM_BASE,	\
					TZRAM_SIZE,	\
					MT_MEMORY | MT_RW | MT_SECURE)

#define MAP_DEVICE MAP_REGION_FLAT(MTK_DEV_BASE, \
				   MTK_DEV_SIZE, \
				   MT_DEVICE | MT_RW | MT_SECURE)

#endif
