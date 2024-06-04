// SPDX-License-Identifier: GPL-2.0
/*************************************************
 * FILE NAME:  air_en8811h.c
 * PURPOSE:
 *      EN8811H PHY Driver for Uboot
 * NOTES:
 *
 *  Copyright (C) 2023 Airoha Technology Corp.
 *************************************************/

/* INCLUDE FILE DECLARATIONS
*/
#include <common.h>
#include <eth_phy.h>
#include <phy.h>
#include <errno.h>
#include <malloc.h>
#include <version.h>
#include "air_en8811h.h"

#ifdef CONFIG_PHY_AIROHA_FW_IN_UBI
#include <ubi_uboot.h>
#endif

#ifdef CONFIG_PHY_AIROHA_FW_IN_MMC
#include <mmc.h>
#endif

#ifdef CONFIG_PHY_AIROHA_FW_IN_MTD
#include <mtd.h>
#endif

#if CONFIG_PHY_AIROHA_FW_BUILTIN
#include "air_en8811h_fw.h"
#endif

#if AIR_UBOOT_REVISION > 0x202004
#include <linux/delay.h>
#endif

/**************************
 * GPIO5  <-> BASE_T_LED0,
 * GPIO4  <-> BASE_T_LED1,
 * GPIO3  <-> BASE_T_LED2,
 **************************/
/* User-defined.B */
#define AIR_LED_SUPPORT
#ifdef AIR_LED_SUPPORT
static const struct air_base_t_led_cfg_s led_cfg[3] = {
/*********************************************************************
 *Enable,   GPIO,        LED Polarity,     LED ON,      LED Blink
**********************************************************************/
	{1, AIR_LED0_GPIO5, AIR_ACTIVE_HIGH, AIR_LED0_ON, AIR_LED0_BLK},
	{1, AIR_LED1_GPIO4, AIR_ACTIVE_HIGH, AIR_LED1_ON, AIR_LED1_BLK},
	{1, AIR_LED2_GPIO3, AIR_ACTIVE_HIGH, AIR_LED2_ON, AIR_LED2_BLK},
};
static const u16 led_dur = UNIT_LED_BLINK_DURATION << AIR_LED_BLK_DUR_64M;
#endif
/* User-defined.E */
/*************************************************************
 *                       F U N C T I O N S
 **************************************************************/
/* Use default PBUS_PHY_ID */
/* EN8811H BUCK write function */
static int air_buckpbus_reg_write(struct phy_device *phydev, unsigned long pbus_address, unsigned int pbus_data)
{
	int ret = 0;

	/* page 4 */
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, (unsigned int)4);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x10, (unsigned int)0);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x11, (unsigned int)((pbus_address >> 16) & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x12, (unsigned int)(pbus_address & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x13, (unsigned int)((pbus_data >> 16) & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x14, (unsigned int)(pbus_data & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	return 0;
}

/* EN8811H BUCK read function */
static unsigned int air_buckpbus_reg_read(struct phy_device *phydev, unsigned long pbus_address)
{
	unsigned int pbus_data = 0, pbus_data_low, pbus_data_high;
	int ret = 0;

	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, (unsigned int)4); /* page 4 */
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return PBUS_INVALID_DATA;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x10, (unsigned int)0);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return PBUS_INVALID_DATA;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x15, (unsigned int)((pbus_address >> 16) & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return PBUS_INVALID_DATA;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x16, (unsigned int)(pbus_address & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return PBUS_INVALID_DATA;
	}

	pbus_data_high = phy_read(phydev, MDIO_DEVAD_NONE, 0x17);
	pbus_data_low = phy_read(phydev, MDIO_DEVAD_NONE, 0x18);
	pbus_data = (pbus_data_high << 16) + pbus_data_low;
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, (unsigned int)0);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	return pbus_data;
}

static int MDIOWriteBuf(struct phy_device *phydev, unsigned long address, unsigned long array_size, const unsigned char *buffer)
{
	unsigned int write_data, offset ;
	int ret = 0;

	/* page 4 */
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, (unsigned int)4);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	/* address increment*/
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x10, (unsigned int)0x8000);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x11, (unsigned int)((address >> 16) & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x12, (unsigned int)(address & 0xffff));
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}

	for (offset = 0; offset < array_size; offset += 4) {
		write_data = (buffer[offset + 3] << 8) | buffer[offset + 2];
		ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x13, write_data);
		if (ret < 0) {
			printf("phy_write, ret: %d\n", ret);
			return ret;
		}
		write_data = (buffer[offset + 1] << 8) | buffer[offset];
		ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x14, write_data);
		if (ret < 0) {
			printf("phy_write, ret: %d\n", ret);
			return ret;
		}
	}
	ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, (unsigned int)0);
	if (ret < 0) {
		printf("phy_write, ret: %d\n", ret);
		return ret;
	}
	return 0;
}

#ifdef  AIR_LED_SUPPORT
static int airoha_led_set_usr_def(struct phy_device *phydev, u8 entity, int polar,
				  u16 on_evt, u16 blk_evt)
{
	int ret = 0;

	if (AIR_ACTIVE_HIGH == polar)
		on_evt |= LED_ON_POL;
	else
		on_evt &= ~LED_ON_POL;

	ret = phy_write_mmd(phydev, 0x1f, LED_ON_CTRL(entity), on_evt | LED_ON_EN);
	if (ret < 0)
		return ret;
	ret = phy_write_mmd(phydev, 0x1f, LED_BLK_CTRL(entity), blk_evt);
	if (ret < 0)
		return ret;
	return 0;
}

static int airoha_led_set_mode(struct phy_device *phydev, u8 mode)
{
	u16 cl45_data;
	int err = 0;

	cl45_data = phy_read_mmd(phydev, 0x1f, LED_BCR);
	switch (mode) {
	case AIR_LED_MODE_DISABLE:
		cl45_data &= ~LED_BCR_EXT_CTRL;
		cl45_data &= ~LED_BCR_MODE_MASK;
		cl45_data |= LED_BCR_MODE_DISABLE;
		break;
	case AIR_LED_MODE_USER_DEFINE:
		cl45_data |= LED_BCR_EXT_CTRL;
		cl45_data |= LED_BCR_CLK_EN;
		break;
	default:
		printf("LED mode%d is not supported!\n", mode);
		return -EINVAL;
	}
	err = phy_write_mmd(phydev, 0x1f, LED_BCR, cl45_data);
	if (err < 0)
		return err;
	return 0;
}

static int airoha_led_set_state(struct phy_device *phydev, u8 entity, u8 state)
{
	u16 cl45_data;
	int err;

	cl45_data = phy_read_mmd(phydev, 0x1f, LED_ON_CTRL(entity));
	if (LED_ENABLE == state)
		cl45_data |= LED_ON_EN;
	else
		cl45_data &= ~LED_ON_EN;

	err = phy_write_mmd(phydev, 0x1f, LED_ON_CTRL(entity), cl45_data);
	if (err < 0)
		return err;
	return 0;
}

static int en8811h_led_init(struct phy_device *phydev)
{
	unsigned int led_gpio = 0, reg_value = 0;
	u16 cl45_data = led_dur;
	int ret, led_id;

	cl45_data = UNIT_LED_BLINK_DURATION << AIR_LED_BLK_DUR_64M;
	ret = phy_write_mmd(phydev, 0x1f, LED_BLK_DUR, cl45_data);
	if (ret < 0)
		return ret;
	cl45_data >>= 1;
	ret = phy_write_mmd(phydev, 0x1f, LED_ON_DUR, cl45_data);
	if (ret < 0)
		return ret;

	ret = airoha_led_set_mode(phydev, AIR_LED_MODE_USER_DEFINE);
	if (ret != 0) {
		printf("LED fail to set mode, ret %d !\n", ret);
		return ret;
	}
	for(led_id = 0; led_id < EN8811H_LED_COUNT; led_id++)
	{
		/* LED0 <-> GPIO5, LED1 <-> GPIO4, LED0 <-> GPIO3 */
		if ( led_cfg[led_id].gpio != (led_id + (AIR_LED0_GPIO5 - (2 * led_id)))) {
			printf("LED%d uses incorrect GPIO%d !\n", led_id, led_cfg[led_id].gpio);
			return -EINVAL;
		}
		reg_value = 0;
		if (led_cfg[led_id].en == LED_ENABLE)
		{
			led_gpio |= BIT(led_cfg[led_id].gpio);
			ret = airoha_led_set_state(phydev, led_id, led_cfg[led_id].en);
			if (ret != 0) {
				printf("LED fail to set state, ret %d !\n", ret);
				return ret;
			}
			ret = airoha_led_set_usr_def(phydev, led_id, led_cfg[led_id].pol, led_cfg[led_id].on_cfg, led_cfg[led_id].blk_cfg);
			if (ret != 0) {
				printf("LED fail to set default, ret %d !\n", ret);
				return ret;
			}
		}
	}
	ret = air_buckpbus_reg_write(phydev, 0xcf8b8, led_gpio);
	if (ret < 0)
		return ret;
	printf("LED initialize OK !\n");
	return 0;
}
#endif /* AIR_LED_SUPPORT */

static char *firmware_buf;
static int en8811h_load_firmware(struct phy_device *phydev)
{
	u32 pbus_value;
	int ret = 0;

	//printf("EN8811H driver load_firmware.\n");
	if (!firmware_buf) {
		firmware_buf = malloc(EN8811H_MD32_DM_SIZE + EN8811H_MD32_DSP_SIZE);
		if (!firmware_buf) {
			printf("[Airoha] cannot allocated buffer for firmware.\n");
			return -ENOMEM;
		}

#if CONFIG_PHY_AIROHA_FW_BUILTIN
	//firmware_buf=EthMD32_dm;
	memcpy(firmware_buf,EthMD32_dm,EN8811H_MD32_DM_SIZE+EN8811H_MD32_DSP_SIZE);
	//memcpy((void *)(firmware_buf+EN8811H_MD32_DM_SIZE),EthMD32_pm,EthMD32_pm_size);
#elif CONFIG_PHY_AIROHA_FW_IN_UBI
		ret = ubi_volume_read("en8811h-fw", firmware_buf, EN8811H_MD32_DM_SIZE + EN8811H_MD32_DSP_SIZE);
		if (ret) {
			printf("[Airoha] read firmware from UBI failed.\n");
			free(firmware_buf);
			firmware_buf = NULL;
			return ret;
		}
#elif defined(CONFIG_PHY_AIROHA_FW_IN_MMC)
		struct mmc *mmc = find_mmc_device(0);
		if (!mmc) {
			printf("[Airoha] opening MMC device failed.\n");
			free(firmware_buf);
			firmware_buf = NULL;
			return -ENODEV;
		}
		if (mmc_init(mmc)) {
			printf("[Airoha] initializing MMC device failed.\n");
			free(firmware_buf);
			firmware_buf = NULL;
			return -ENODEV;
		}
		if (IS_SD(mmc)) {
			printf("[Airoha] SD card is not supported.\n");
			free(firmware_buf);
			firmware_buf = NULL;
			return -EINVAL;
		}
		ret = mmc_set_part_conf(mmc, 1, 2, 2);
		if (ret) {
			printf("[Airoha] cannot access eMMC boot1 hw partition.\n");
			free(firmware_buf);
			firmware_buf = NULL;
			return ret;
		}
		ret = blk_dread(mmc_get_blk_desc(mmc), 0, 0x120, firmware_buf);
		mmc_set_part_conf(mmc, 1, 1, 0);
		if (ret != 0x120) {
			printf("[Airoha] cannot read firmware from eMMC.\n");
			free(firmware_buf);
			firmware_buf = NULL;
			return -EIO;
		}
#else
#warning EN8811H firmware loading not implemented
		free(firmware_buf);
		firmware_buf = NULL;
		return -EOPNOTSUPP;
#endif
	}

	ret = air_buckpbus_reg_write(phydev, EN8811H_FW_CTRL_1, EN8811H_FW_CTRL_1_START);
	if (ret < 0)
		return ret;
	pbus_value = air_buckpbus_reg_read(phydev, EN8811H_FW_CTRL_2);
	pbus_value |= EN8811H_FW_CTRL_2_LOADING;
	ret = air_buckpbus_reg_write(phydev, EN8811H_FW_CTRL_2, pbus_value);
	if (ret < 0)
		return ret;

	/* Download DM */
	ret = MDIOWriteBuf(phydev, 0x00000000, EN8811H_MD32_DM_SIZE, firmware_buf);
	if (ret < 0) {
		printf("[Airoha] MDIOWriteBuf 0x00000000 fail.\n");
		return ret;
	}
	/* Download PM */
	ret = MDIOWriteBuf(phydev, 0x00100000, EN8811H_MD32_DSP_SIZE, firmware_buf + EN8811H_MD32_DM_SIZE);
	if (ret < 0) {
		printf("[Airoha] MDIOWriteBuf 0x00100000 fail.\n");
		return ret;
	}
	pbus_value = air_buckpbus_reg_read(phydev, EN8811H_FW_CTRL_2);
	pbus_value &= ~EN8811H_FW_CTRL_2_LOADING;
	ret = air_buckpbus_reg_write(phydev, EN8811H_FW_CTRL_2, pbus_value);
	if (ret < 0)
		return ret;
	ret = air_buckpbus_reg_write(phydev, EN8811H_FW_CTRL_1, EN8811H_FW_CTRL_1_FINISH);
	if (ret < 0)
		return ret;

	return 0;
}

static int en8811h_config(struct phy_device *phydev)
{
	int pid1 = 0, pid2 = 0;

	printf("EN8811H driver config.\n");
	pid1 = phy_read(phydev, MDIO_DEVAD_NONE, MII_PHYSID1);
	pid2 = phy_read(phydev, MDIO_DEVAD_NONE, MII_PHYSID2);
	if ((EN8811H_PHY_ID1 != pid1) || (EN8811H_PHY_ID2 != pid2)) {
		printf("EN8811H does not exist !\n");
		return -ENODEV;
	}

	return 0;
}

static int en8811h_get_autonego(struct phy_device *phydev, int *an)
{
	int reg;
	reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);
	if (reg < 0)
		return -EINVAL;
	if (reg & BMCR_ANENABLE)
		*an = AUTONEG_ENABLE;
	else
		*an = AUTONEG_DISABLE;
	return 0;
}

static int en8811h_restart_host(struct phy_device *phydev)
{
	int ret;

	ret = air_buckpbus_reg_write(phydev, EN8811H_FW_CTRL_1,
				     EN8811H_FW_CTRL_1_START);
	if (ret < 0)
		return ret;

	return air_buckpbus_reg_write(phydev, EN8811H_FW_CTRL_1,
				      EN8811H_FW_CTRL_1_FINISH);
}

static int en8811h_startup(struct phy_device *phydev)
{
	//printf("EN8811H driver startup.\n");
	ofnode node = phy_get_ofnode(phydev);
	struct en8811h_priv *priv = phydev->priv;
	int ret = 0, lpagb = 0, lpa = 0, common_adv_gb = 0, common_adv = 0, advgb = 0, adv = 0, reg = 0, an = AUTONEG_DISABLE, bmcr = 0, reg_value;
	//int old_link = phydev->link;
	u32 pbus_value = 0, retry;

	eth_phy_reset(phydev->dev, 1);
	mdelay(10);
	eth_phy_reset(phydev->dev, 0);
	mdelay(1);

	if (!priv->firmware_version) {
		ret = en8811h_load_firmware(phydev);
		if (ret) {
			printf("EN8811H load firmware fail.\n");
			return ret;
		}
	} else {
		ret=en8811h_restart_host(phydev);
		if (ret) {
			printf("EN8811H restart host fail.\n");
			return ret;
		}
	}
	retry = MAX_RETRY;
	do {
		mdelay(300);
		reg_value = phy_read_mmd(phydev, 0x1e, 0x8009);
		if (EN8811H_PHY_READY == reg_value) {
			printf("EN8811H PHY ready!\n");
			break;
		}
		retry--;
	} while (retry);
	if (0 == retry) {
		printf("EN8811H PHY is not ready. (MD32 FW Status reg: 0x%x)\n", reg_value);
		pbus_value = air_buckpbus_reg_read(phydev, 0x3b3c);
		printf("Check MD32 FW Version(0x3b3c) : %08x\n", pbus_value);
		printf("EN8811H initialize fail!\n");
		return 0;
	}
	/* Mode selection*/
	//printf("EN8811H Mode 1 !\n");
	ret = phy_write_mmd(phydev, 0x1e, 0x800c, 0x0);
	if (ret < 0)
		return ret;
	ret = phy_write_mmd(phydev, 0x1e, 0x800d, 0x0);
	if (ret < 0)
		return ret;
	ret = phy_write_mmd(phydev, 0x1e, 0x800e, 0x1101);
	if (ret < 0)
		return ret;
	ret = phy_write_mmd(phydev, 0x1e, 0x800f, 0x0002);
	if (ret < 0)
		return ret;

	/* Serdes polarity */
	pbus_value = air_buckpbus_reg_read(phydev, 0xca0f8);
	pbus_value &= 0xfffffffc;
	pbus_value |= ofnode_read_bool(node, "airoha,rx-pol-reverse") ?
			EN8811H_RX_POLARITY_REVERSE : EN8811H_RX_POLARITY_NORMAL;
	pbus_value |= ofnode_read_bool(node, "airoha,tx-pol-reverse") ?
			EN8811H_TX_POLARITY_REVERSE : EN8811H_TX_POLARITY_NORMAL;
	ret = air_buckpbus_reg_write(phydev, 0xca0f8, pbus_value);
	if (ret < 0)
		return ret;
	pbus_value = air_buckpbus_reg_read(phydev, 0xca0f8);
	printf("Tx, Rx Polarity(0xca0f8): %08x\n", pbus_value);
	pbus_value = air_buckpbus_reg_read(phydev, 0x3b3c);
	printf("MD32 FW Version(0x3b3c) : %08x\n", pbus_value);
	if (!priv->firmware_version) {priv->firmware_version=pbus_value;}
#if defined(AIR_LED_SUPPORT)
	ret = en8811h_led_init(phydev);
	if (ret < 0) {
		printf("en8811h_led_init fail\n");
	}
#endif
	printf("EN8811H initialize OK ! (drv ver: %s)\n", EN8811H_DRIVER_VERSION);

	ret = genphy_update_link(phydev);
	if (ret)
	{
		printf("ret %d!\n", ret);
		return ret;
	}

	ret = genphy_parse_link(phydev);
	if (ret)
	{
		printf("ret %d!\n", ret);
		return ret;
	}

	//if (old_link && phydev->link)
	//   return 0;

	phydev->supported = ( SUPPORTED_1000baseT_Full | SUPPORTED_1000baseT_Half);
	phydev->speed = SPEED_100;
	phydev->duplex = DUPLEX_FULL;
	phydev->pause = 0;
	phydev->asym_pause = 0;

	reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
	if (reg < 0)
	{
		printf("MII_BMSR reg %d!\n", reg);
		return reg;
	}
	reg = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMSR);
	if (reg < 0)
	{
		printf("MII_BMSR reg %d!\n", reg);
		return reg;
	}
	if(reg & BMSR_LSTATUS)
	{
		pbus_value = air_buckpbus_reg_read(phydev, 0x109D4);
		if (0x10 & pbus_value) {
			printf("SPEED 2500 FDX!\n");
			phydev->speed = SPEED_2500;
			phydev->duplex = DUPLEX_FULL;
		}
		else
		{
			ret = en8811h_get_autonego(phydev, &an);
			if ((AUTONEG_ENABLE == an) && (0 == ret))
			{
				printf("AN mode...\n");
				lpagb = phy_read(phydev, MDIO_DEVAD_NONE, MII_STAT1000);
				if (lpagb < 0 )
					return lpagb;
				advgb = phy_read(phydev, MDIO_DEVAD_NONE, MII_CTRL1000);
				if (advgb < 0 )
					return advgb;
				common_adv_gb = (lpagb & (advgb << 2));

				lpa = phy_read(phydev, MDIO_DEVAD_NONE, MII_LPA);
				if (lpa < 0 )
					return lpa;
				adv = phy_read(phydev, MDIO_DEVAD_NONE, MII_ADVERTISE);
				if (adv < 0 )
					return adv;
				common_adv = (lpa & adv);

				phydev->speed = SPEED_10;
				phydev->duplex = DUPLEX_HALF;
				if (common_adv_gb & (LPA_1000FULL | LPA_1000HALF))
				{
					printf("AN mode...SPEED 1000!\n");
					phydev->speed = SPEED_1000;
					if (common_adv_gb & LPA_1000FULL)
						phydev->duplex = DUPLEX_FULL;
				}
				else if (common_adv & (LPA_100FULL | LPA_100HALF))
				{
					printf("AN mode...SPEED 100!\n");
					phydev->speed = SPEED_100;
					if (common_adv & LPA_100FULL)
						phydev->duplex = DUPLEX_FULL;
				}
				else
				{
					printf("AN mode...SPEED 10!\n");
					if (common_adv & LPA_10FULL)
						phydev->duplex = DUPLEX_FULL;
				}
			}
			else
			{
				printf("Force mode!\n");
				bmcr = phy_read(phydev, MDIO_DEVAD_NONE, MII_BMCR);

				if (bmcr < 0)
					return bmcr;

				if (bmcr & BMCR_FULLDPLX)
					phydev->duplex = DUPLEX_FULL;
				else
					phydev->duplex = DUPLEX_HALF;

				if (bmcr & BMCR_SPEED1000)
					phydev->speed = SPEED_1000;
				else if (bmcr & BMCR_SPEED100)
					phydev->speed = SPEED_100;
				else
					phydev->speed = SPEED_100;
			}
		}
	}

	return ret;
}

static int en8811h_probe(struct phy_device *phydev)
{
	struct en8811h_priv *priv;

	priv=malloc(sizeof(*priv));
	if (!priv)
		return -ENOMEM;
	memset(priv,0,sizeof(*priv));

	phydev->priv = priv;

	return 0;
}
#if AIR_UBOOT_REVISION > 0x202303
U_BOOT_PHY_DRIVER(en8811h) = {
	.name = "Airoha EN8811H",
	.uid = EN8811H_PHY_ID,
	.mask = 0x0ffffff0,
	.config = &en8811h_config,
	.probe = &en8811h_probe,
	.startup = &en8811h_startup,
	.shutdown = &genphy_shutdown,
};
#else
static struct phy_driver AIR_EN8811H_driver = {
	.name = "Airoha EN8811H",
	.uid = EN8811H_PHY_ID,
	.mask = 0x0ffffff0,
	.config = &en8811h_config,
	.startup = &en8811h_startup,
	.startup = &en8811h_startup,
	.shutdown = &genphy_shutdown,
};

int phy_air_en8811h_init(void)
{
	phy_register(&AIR_EN8811H_driver);
	return 0;
}
#endif
