// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2022 MediaTek Inc.
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <asm/io.h>
#include <asm/global_data.h>
#include <linux/sizes.h>
#include <linux/types.h>

DECLARE_GLOBAL_DATA_PTR;

int board_init(void)
{
	return 0;
}

#define	MT7988_BOOT_NOR		0
#define	MT7988_BOOT_SPIM_NAND	1
#define	MT7988_BOOT_EMMC	2
#define	MT7988_BOOT_SNFI_NAND	3

const char *mtk_board_rootdisk(void)
{
	switch ((readl(0x1001f6f0) & 0xc00) >> 10) {
	case MT7988_BOOT_NOR:
		return "nor";

	case MT7988_BOOT_SPIM_NAND:
		return "spim-nand";

	case MT7988_BOOT_EMMC:
		return "emmc";

	case MT7988_BOOT_SNFI_NAND:
		return "sd";

	default:
		return "";
	}
}

ulong board_get_load_addr(void)
{
	if (gd->ram_size <= SZ_128M)
		return gd->ram_base;

	if (gd->ram_size <= SZ_256M)
		return gd->ram_top - SZ_64M;

	return gd->ram_base + SZ_256M;
}

const static struct {
	const char *name;
	const char *desc;
	int group;
} board_fit_conf_info[] = {
	{ "mt7988a-rfb-emmc", "Image on eMMC", 0 },
	{ "mt7988a-rfb-sd", "Image on SD", 0 },
	{ "mt7988a-rfb-snfi-nand", "Image on SNFI-NAND", 0 },
	{ "mt7988a-rfb-spim-nand", "Image on SPIM-NAND (UBI)", 0 },
	{ "mt7988a-rfb-spim-nand-factory", "UBI \"factory\" volume config", -1 },
	{ "mt7988a-rfb-spim-nand-nmbm", "Image on SPIM-NAND (NMBM)", 0 },
	{ "mt7988a-rfb-spim-nor", "Image on SPI-NOR", 0 },
	{ "mt7988a-rfb-eth1-aqr", "eth1 with AQR113C 10Gbps PHY", 1 },
	{ "mt7988a-rfb-eth1-cux3410", "eth1 with CUX3410 10Gbps PHY", 1 },
	{ "mt7988a-rfb-eth1-i2p5g-phy", "eth1 with internal 2.5Gbps PHY", 1 },
	{ "mt7988a-rfb-eth1-mxl", "eth1 with GPY211 2.5Gbps PHY", 1 },
	{ "mt7988a-rfb-eth1-sfp", "eth1 uses SFP interface", 1 },
	{ "mt7988a-rfb-eth2-aqr", "eth2 with AQR113C 10Gbps PHY", 2 },
	{ "mt7988a-rfb-eth2-cux3410", "eth2 with CUX3410 10Gbps PHY", 2 },
	{ "mt7988a-rfb-eth2-mxl", "eth2 with GPY211 2.5Gbps PHY", 2 },
	{ "mt7988a-rfb-eth2-sfp", "eth2 uses SFP interface", 2 },
	{ "mt7988a-rfb-spidev", "spi1 with DH2228FV", -1 },
};

int mtk_board_get_fit_conf_info(const char *name, const char **retdesc)
{
	uint32_t i;

	for (i = 0; i < ARRAY_SIZE(board_fit_conf_info); i++) {
		if (!strcmp(name, board_fit_conf_info[i].name)) {
			(*retdesc) = board_fit_conf_info[i].desc;
			return board_fit_conf_info[i].group;
		}
	}

	(*retdesc) = NULL;
	return -1;
}
