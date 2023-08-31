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
#include <phy.h>
#include <errno.h>
#include <version.h>
#include "air_en8811h.h"
#include "air_en8811h_fw.h"

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
/* Airoha MII read function */
static int air_mii_cl22_read(struct mii_dev *bus, int phy_addr, int phy_register)
{
    int read_data = bus->read(bus, phy_addr, MDIO_DEVAD_NONE, phy_register);

    if (read_data < 0)
        return -EIO;
    return read_data;
}

/* Airoha MII write function */
static int air_mii_cl22_write(struct mii_dev *bus, int phy_addr, int phy_register, int write_data)
{
    int ret = 0;

    ret = bus->write(bus, phy_addr, MDIO_DEVAD_NONE, phy_register, write_data);
    if (ret < 0) {
        printf("bus->write, ret: %d\n", ret);
        return ret;
    }
    return ret;
}

static int air_mii_cl45_read(struct phy_device *phydev, int devad, u16 reg)
{
    int ret = 0;
    int data;

    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, devad);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return INVALID_DATA;
    }
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG, reg);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return INVALID_DATA;
    }
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, MMD_OP_MODE_DATA | devad);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return INVALID_DATA;
    }
    data = phy_read(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG);
    return data;
}

static int air_mii_cl45_write(struct phy_device *phydev, int devad, u16 reg, u16 write_data)
{
    int ret = 0;

    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, devad);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return ret;
    }
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG, reg);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return ret;
    }
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, MMD_OP_MODE_DATA | devad);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return ret;
    }
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG, write_data);
    if (ret < 0) {
        printf("phy_write, ret: %d\n", ret);
        return ret;
    }
    return 0;
}
/* Use default PBUS_PHY_ID */
/* EN8811H PBUS write function */
static int air_pbus_reg_write(struct phy_device *phydev, unsigned long pbus_address, unsigned long pbus_data)
{
    int ret = 0;
    struct mii_dev *mbus = phydev->bus;

    ret = air_mii_cl22_write(mbus, ((phydev->addr) + 8), 0x1F, (unsigned int)(pbus_address >> 6));
    if (ret < 0)
        return ret;
    ret = air_mii_cl22_write(mbus, ((phydev->addr) + 8), (unsigned int)((pbus_address >> 2) & 0xf), (unsigned int)(pbus_data & 0xFFFF));
    if (ret < 0)
        return ret;
    ret = air_mii_cl22_write(mbus, ((phydev->addr) + 8), 0x10, (unsigned int)(pbus_data >> 16));
    if (ret < 0)
        return ret;
    return 0;
}

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

    ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, (unsigned int)4);        /* page 4 */
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

    ret = air_mii_cl45_write(phydev, 0x1f, LED_ON_CTRL(entity), on_evt | LED_ON_EN);
    if (ret < 0)
        return ret;
    ret = air_mii_cl45_write(phydev, 0x1f, LED_BLK_CTRL(entity), blk_evt);
    if (ret < 0)
        return ret;
    return 0;
}

static int airoha_led_set_mode(struct phy_device *phydev, u8 mode)
{
    u16 cl45_data;
    int err = 0;

    cl45_data = air_mii_cl45_read(phydev, 0x1f, LED_BCR);
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
    err = air_mii_cl45_write(phydev, 0x1f, LED_BCR, cl45_data);
    if (err < 0)
        return err;
    return 0;
}

static int airoha_led_set_state(struct phy_device *phydev, u8 entity, u8 state)
{
    u16 cl45_data;
    int err;

    cl45_data = air_mii_cl45_read(phydev, 0x1f, LED_ON_CTRL(entity));
    if (LED_ENABLE == state)
        cl45_data |= LED_ON_EN;
    else
        cl45_data &= ~LED_ON_EN;

    err = air_mii_cl45_write(phydev, 0x1f, LED_ON_CTRL(entity), cl45_data);
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
    ret = air_mii_cl45_write(phydev, 0x1f, LED_BLK_DUR, cl45_data);
    if (ret < 0)
        return ret;
    cl45_data >>= 1;
    ret = air_mii_cl45_write(phydev, 0x1f, LED_ON_DUR, cl45_data);
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

static int en8811h_load_firmware(struct phy_device *phydev)
{
    int ret = 0;
    u32 pbus_value = 0;

    ret = air_buckpbus_reg_write(phydev, 0x0f0018, 0x0);
    if (ret < 0)
        return ret;
    pbus_value = air_buckpbus_reg_read(phydev, 0x800000);
    pbus_value |= BIT(11);
    ret = air_buckpbus_reg_write(phydev, 0x800000, pbus_value);
    if (ret < 0)
        return ret;
    /* Download DM */
    ret = MDIOWriteBuf(phydev, 0x00000000, EthMD32_dm_size, EthMD32_dm);
	if (ret < 0) {
        printf("[Airoha] MDIOWriteBuf 0x00000000 fail.\n");
        return ret;
    }
    /* Download PM */
    ret = MDIOWriteBuf(phydev, 0x00100000, EthMD32_pm_size, EthMD32_pm);
	if (ret < 0) {
        printf("[Airoha] MDIOWriteBuf 0x00100000 fail.\n");
        return ret;
    }
    pbus_value = air_buckpbus_reg_read(phydev, 0x800000);
    pbus_value &= ~BIT(11);
    ret = air_buckpbus_reg_write(phydev, 0x800000, pbus_value);
    if (ret < 0)
        return ret;
    ret = air_buckpbus_reg_write(phydev, 0x0f0018, 0x01);
    if (ret < 0)
        return ret;
    return 0;
}

static int en8811h_config(struct phy_device *phydev)
{
    int ret = 0;
    int reg_value, pid1 = 0, pid2 = 0;
    u32 pbus_value, retry;

    ret = air_pbus_reg_write(phydev, 0xcf928 , 0x0);
    if (ret < 0)
        return ret;

    pid1 = phy_read(phydev, MDIO_DEVAD_NONE, MII_PHYSID1);
    pid2 = phy_read(phydev, MDIO_DEVAD_NONE, MII_PHYSID2);
    printf("PHY = %x - %x\n", pid1, pid2);
    if ((EN8811H_PHY_ID1 != pid1) || (EN8811H_PHY_ID2 != pid2)) {
        printf("EN8811H dose not exist !\n");
        return -ENODEV;
    }
    ret = en8811h_load_firmware(phydev);
    if (ret) {
        printf("EN8811H load firmware fail.\n");
        return ret;
    }
    retry = MAX_RETRY;
    do {
        mdelay(300);
        reg_value = air_mii_cl45_read(phydev, 0x1e, 0x8009);
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
    printf("EN8811H Mode 1 !\n");
    ret = air_mii_cl45_write(phydev, 0x1e, 0x800c, 0x0);
    if (ret < 0)
        return ret;
    ret = air_mii_cl45_write(phydev, 0x1e, 0x800d, 0x0);
    if (ret < 0)
        return ret;
    ret = air_mii_cl45_write(phydev, 0x1e, 0x800e, 0x1101);
    if (ret < 0)
        return ret;
    ret = air_mii_cl45_write(phydev, 0x1e, 0x800f, 0x0002);
    if (ret < 0)
        return ret;

    /* Serdes polarity */
    pbus_value = air_buckpbus_reg_read(phydev, 0xca0f8);
    pbus_value = (pbus_value & 0xfffffffc) | EN8811H_RX_POLARITY_NORMAL | EN8811H_TX_POLARITY_NORMAL;
    ret = air_buckpbus_reg_write(phydev, 0xca0f8, pbus_value);
    if (ret < 0)
        return ret;
    pbus_value = air_buckpbus_reg_read(phydev, 0xca0f8);
    printf("Tx, Rx Polarity(0xca0f8): %08x\n", pbus_value);
    pbus_value = air_buckpbus_reg_read(phydev, 0x3b3c);
    printf("MD32 FW Version(0x3b3c) : %08x\n", pbus_value);
#if defined(AIR_LED_SUPPORT)
    ret = en8811h_led_init(phydev);
    if (ret < 0) {
        printf("en8811h_led_init fail\n");
    }
#endif
    printf("EN8811H initialize OK ! (%s)\n", EN8811H_DRIVER_VERSION);
    return 0;
}

#if AIR_UBOOT_REVISION > 0x202303
U_BOOT_PHY_DRIVER(en8811h) = {
    .name = "Airoha EN8811H",
    .uid = EN8811H_PHY_ID,
    .mask = 0x0ffffff0,
    .config = &en8811h_config,
    .startup = &genphy_update_link,
    .shutdown = &genphy_shutdown,
};
#else
static struct phy_driver AIR_EN8811H_driver = {
    .name = "Airoha EN8811H",
    .uid = EN8811H_PHY_ID,
    .mask = 0x0ffffff0,
    .config = &en8811h_config,
    .startup = &genphy_update_link,
    .shutdown = &genphy_shutdown,
};

int phy_air_en8811h_init(void)
{
    phy_register(&AIR_EN8811H_driver);
    return 0;
}
#endif