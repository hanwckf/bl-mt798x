// SPDX-License-Identifier: GPL-2.0
/*************************************************
 * FILE NAME:  air_en8801s.c
 * PURPOSE:
 *      EN8801S PHY Driver for Uboot
 * NOTES:
 *
 *  Copyright (C) 2023 Airoha Technology Corp.
 *************************************************/

/* INCLUDE FILE DECLARATIONS
 */
#include <phy.h>
#include <errno.h>
#include <version.h>
#include "air_en8801s.h"

#if AIR_UBOOT_REVISION > 0x202004
#include <linux/delay.h>
#endif

static struct phy_device *s_phydev = 0;
/******************************************************
 * The following led_cfg example is for reference only.
 * LED5 1000M/LINK/ACT   (GPIO5)  <-> BASE_T_LED0,
 * LED6 10/100M/LINK/ACT (GPIO9)  <-> BASE_T_LED1,
 * LED4 100M/LINK/ACT    (GPIO8)  <-> BASE_T_LED2,
 ******************************************************/
/* User-defined.B */
#define AIR_LED_SUPPORT
#ifdef AIR_LED_SUPPORT
static const AIR_BASE_T_LED_CFG_T led_cfg[4] =
{
    /*
     *    LED Enable,     GPIO,       LED Polarity,            LED ON,               LED Blink
     */
         {LED_ENABLE,       5,       AIR_ACTIVE_LOW,      BASE_T_LED0_ON_CFG,    BASE_T_LED0_BLK_CFG}, /* BASE-T LED0 */
         {LED_ENABLE,       9,       AIR_ACTIVE_LOW,      BASE_T_LED1_ON_CFG,    BASE_T_LED1_BLK_CFG}, /* BASE-T LED1 */
         {LED_ENABLE,       8,       AIR_ACTIVE_LOW,      BASE_T_LED2_ON_CFG,    BASE_T_LED2_BLK_CFG}, /* BASE-T LED2 */
         {LED_DISABLE,      1,       AIR_ACTIVE_LOW,      BASE_T_LED3_ON_CFG,    BASE_T_LED3_BLK_CFG}  /* BASE-T LED3 */
};
static const u16 led_dur = UNIT_LED_BLINK_DURATION << AIR_LED_BLK_DUR_64M;
#endif
/* User-defined.E */
/************************************************************************
 *                  F U N C T I O N S
 ************************************************************************/
/* Airoha MII read function */
static int airoha_cl22_read(struct mii_dev *bus, int phy_addr, int phy_register)
{
    int read_data = bus->read(bus, phy_addr, MDIO_DEVAD_NONE, phy_register);

    if (read_data < 0)
        return -EIO;
    return read_data;
}

/* Airoha MII write function */
static int airoha_cl22_write(struct mii_dev *bus, int phy_addr, int phy_register, int write_data)
{
    int ret = bus->write(bus, phy_addr, MDIO_DEVAD_NONE, phy_register, write_data);

    return ret;
}

static int airoha_cl45_write(struct phy_device *phydev, int devad, int reg, int val)
{
    int ret = 0;

    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, devad);
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG, reg);
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, MMD_OP_MODE_DATA | devad);
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG, val);
    AIR_RTN_ERR(ret);
    return ret;
}

static int airoha_cl45_read(struct phy_device *phydev, int devad, int reg)
{
    int read_data, ret;

    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, devad);
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG, reg);
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, MII_MMD_ACC_CTL_REG, MMD_OP_MODE_DATA | devad);
    AIR_RTN_ERR(ret);
    read_data = phy_read(phydev, MDIO_DEVAD_NONE, MII_MMD_ADDR_DATA_REG);
    if (read_data < 0)
        return -EIO;
    return read_data;
}

/* EN8801 PBUS write function */
int airoha_pbus_write(struct mii_dev *bus, int pbus_addr, int pbus_reg, unsigned long pbus_data)
{
    int ret = 0;

    ret = airoha_cl22_write(bus, pbus_addr, 0x1F, (pbus_reg >> 6));
    AIR_RTN_ERR(ret);
    ret = airoha_cl22_write(bus, pbus_addr, ((pbus_reg >> 2) & 0xf), (pbus_data & 0xFFFF));
    AIR_RTN_ERR(ret);
    ret = airoha_cl22_write(bus, pbus_addr, 0x10, (pbus_data >> 16));
    AIR_RTN_ERR(ret);
    return ret;
}

/* EN8801 PBUS read function */
unsigned long airoha_pbus_read(struct mii_dev *bus, int pbus_addr, int pbus_reg)
{
    unsigned long pbus_data;
    unsigned int pbus_data_low, pbus_data_high;

    airoha_cl22_write(bus, pbus_addr, 0x1F, (pbus_reg >> 6));
    pbus_data_low = airoha_cl22_read(bus, pbus_addr, ((pbus_reg >> 2) & 0xf));
    pbus_data_high = airoha_cl22_read(bus, pbus_addr, 0x10);
    pbus_data = (pbus_data_high << 16) + pbus_data_low;
    return pbus_data;
}

/* Airoha Token Ring Write function */
static int airoha_tr_reg_write(struct phy_device *phydev, unsigned long tr_address, unsigned long tr_data)
{
    int ret;

    ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, 0x52b5);       /* page select */
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x11, (int)(tr_data & 0xffff));
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x12, (int)(tr_data >> 16));
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x10, (int)(tr_address | TrReg_WR));
    AIR_RTN_ERR(ret);
    ret = phy_write(phydev, MDIO_DEVAD_NONE, 0x1F, 0x0);          /* page resetore */
    AIR_RTN_ERR(ret);
    return ret;
}

int airoha_phy_process(void)
{
    int ret = 0, pbus_addr = EN8801S_PBUS_PHY_ID;
    unsigned long pbus_data;
    struct mii_dev *mbus;

    mbus = s_phydev->bus;
    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x19e0);
    pbus_data |= BIT(0);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x19e0, pbus_data);
    if(ret)
        printf("error: airoha_pbus_write fail ret: %d\n", ret);
    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x19e0);
    pbus_data &= ~BIT(0);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x19e0, pbus_data);
    if(ret)
        printf("error: airoha_pbus_write fail ret: %d\n", ret);

    if(ret)
        printf("error: FCM regs reset fail, ret: %d\n", ret);
    else
        debug("FCM regs reset successful\n");
    return ret;
}

#ifdef  AIR_LED_SUPPORT
static int airoha_led_set_usr_def(struct phy_device *phydev, u8 entity, int polar,
                                   u16 on_evt, u16 blk_evt)
{
    int ret = 0;

    if (AIR_ACTIVE_HIGH == polar) {
        on_evt |= LED_ON_POL;
    } else {
        on_evt &= ~LED_ON_POL;
    }
    ret = airoha_cl45_write(phydev, 0x1f, LED_ON_CTRL(entity), on_evt | LED_ON_EN);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1f, LED_BLK_CTRL(entity), blk_evt);
    AIR_RTN_ERR(ret);
    return 0;
}

static int airoha_led_set_mode(struct phy_device *phydev, u8 mode)
{
    u16 cl45_data;
    int err = 0;

    cl45_data = airoha_cl45_read(phydev, 0x1f, LED_BCR);
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
    err = airoha_cl45_write(phydev, 0x1f, LED_BCR, cl45_data);
    AIR_RTN_ERR(err);
    return 0;
}

static int airoha_led_set_state(struct phy_device *phydev, u8 entity, u8 state)
{
    u16 cl45_data;
    int err;

    cl45_data = airoha_cl45_read(phydev, 0x1f, LED_ON_CTRL(entity));
    if (LED_ENABLE == state) {
        cl45_data |= LED_ON_EN;
    } else {
        cl45_data &= ~LED_ON_EN;
    }

    err = airoha_cl45_write(phydev, 0x1f, LED_ON_CTRL(entity), cl45_data);
    AIR_RTN_ERR(err);
    return 0;
}

static int en8801s_led_init(struct phy_device *phydev)
{

    unsigned long led_gpio = 0, reg_value = 0;
    int ret = 0, led_id;
    struct mii_dev *mbus = phydev->bus;
    int gpio_led_rg[3] = {0x1870, 0x1874, 0x1878};
    u16 cl45_data = led_dur;

    ret = airoha_cl45_write(phydev, 0x1f, LED_BLK_DUR, cl45_data);
    AIR_RTN_ERR(ret);
    cl45_data >>= 1;
    ret = airoha_cl45_write(phydev, 0x1f, LED_ON_DUR, cl45_data);
    AIR_RTN_ERR(ret);
    ret = airoha_led_set_mode(phydev, AIR_LED_MODE_USER_DEFINE);
    if (ret != 0) {
        printf("LED fail to set mode, ret %d !\n", ret);
        return ret;
    }
    for(led_id = 0; led_id < EN8801S_LED_COUNT; led_id++) {
        reg_value = 0;
        ret = airoha_led_set_state(phydev, led_id, led_cfg[led_id].en);
        if (ret != 0) {
            printf("LED fail to set state, ret %d !\n", ret);
            return ret;
        }
        if (LED_ENABLE == led_cfg[led_id].en) {
            if ( (led_cfg[led_id].gpio < 0) || led_cfg[led_id].gpio > 9) {
                printf("GPIO%d is out of range!! GPIO number is 0~9.\n", led_cfg[led_id].gpio);
                return -EIO;
            }
            led_gpio |= BIT(led_cfg[led_id].gpio);
            reg_value = airoha_pbus_read(mbus, EN8801S_PBUS_PHY_ID, gpio_led_rg[led_cfg[led_id].gpio / 4]);
            LED_SET_GPIO_SEL(led_cfg[led_id].gpio, led_id, reg_value);
            debug("[Airoha] gpio%d, reg_value 0x%lx\n", led_cfg[led_id].gpio, reg_value);
            ret = airoha_pbus_write(mbus, EN8801S_PBUS_PHY_ID, gpio_led_rg[led_cfg[led_id].gpio / 4], reg_value);
            AIR_RTN_ERR(ret);
            ret = airoha_led_set_usr_def(phydev, led_id, led_cfg[led_id].pol, led_cfg[led_id].on_cfg, led_cfg[led_id].blk_cfg);
            if (ret != 0) {
                printf("LED fail to set usr def, ret %d !\n", ret);
                return ret;
            }
        }
    }
    reg_value = (airoha_pbus_read(mbus, EN8801S_PBUS_PHY_ID, 0x1880) & ~led_gpio);
    ret = airoha_pbus_write(mbus, EN8801S_PBUS_PHY_ID, 0x1880, reg_value);
    AIR_RTN_ERR(ret);
    ret = airoha_pbus_write(mbus, EN8801S_PBUS_PHY_ID, 0x186c, led_gpio);
    AIR_RTN_ERR(ret);

    printf("LED initialize OK !\n");
    return 0;
}
#endif /* AIR_LED_SUPPORT */

static int en8801s_config(struct phy_device *phydev)
{
    int reg_value = 0, ret = 0, version;
    struct mii_dev *mbus = phydev->bus;
    int retry, pbus_addr = EN8801S_PBUS_DEFAULT_ID;
    int phy_addr = EN8801S_MDIO_PHY_ID;
    unsigned long pbus_data = 0;
    gephy_all_REG_LpiReg1Ch      GPHY_RG_LPI_1C;
    gephy_all_REG_dev1Eh_reg324h GPHY_RG_1E_324;
    gephy_all_REG_dev1Eh_reg012h GPHY_RG_1E_012;
    gephy_all_REG_dev1Eh_reg017h GPHY_RG_1E_017;

    s_phydev = phydev;
    retry = MAX_OUI_CHECK;
    while (1) {
        /* PHY OUI */
        pbus_data = airoha_pbus_read(mbus, pbus_addr, EN8801S_RG_ETHER_PHY_OUI);
        if (EN8801S_PBUS_OUI == pbus_data) {
            printf("PBUS addr 0x%x: Start initialized.\n", pbus_addr);
            ret = airoha_pbus_write(mbus, pbus_addr, EN8801S_RG_BUCK_CTL, 0x03);
            AIR_RTN_ERR(ret);
            break;
        } else
            pbus_addr = EN8801S_PBUS_PHY_ID;

        if (0 == --retry) {
            printf("EN8801S Probe fail !\n");
            return 0;
        }
    }

    pbus_data = airoha_pbus_read(mbus, pbus_addr, EN8801S_RG_PROD_VER);
	version = pbus_data & 0xf;
	printf("EN8801S Procduct Version :E%d\n", version);
    /* SMI ADDR */
    pbus_data = airoha_pbus_read(mbus, pbus_addr, EN8801S_RG_SMI_ADDR);
    pbus_data = (pbus_data & 0xffff0000) | (unsigned long)(pbus_addr << 8) | (unsigned long)(EN8801S_MDIO_DEFAULT_ID);
    ret = airoha_pbus_write(mbus, pbus_addr, EN8801S_RG_SMI_ADDR, pbus_data);
    AIR_RTN_ERR(ret);
    mdelay(10);

    pbus_data = (airoha_pbus_read(mbus, pbus_addr, EN8801S_RG_LTR_CTL) & (~0x3)) | BIT(2) ;
    ret = airoha_pbus_write(mbus, pbus_addr, EN8801S_RG_LTR_CTL, pbus_data);
    AIR_RTN_ERR(ret);
    mdelay(500);
    pbus_data = (pbus_data & ~BIT(2)) | EN8801S_RX_POLARITY_NORMAL | EN8801S_TX_POLARITY_NORMAL;
    ret = airoha_pbus_write(mbus, pbus_addr, EN8801S_RG_LTR_CTL, pbus_data);
    AIR_RTN_ERR(ret);
    mdelay(500);
    if (version == 4) {
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1900);
        /* printf("Before 0x1900 0x%x\n", pbus_data); */
        ret = airoha_pbus_write(mbus, pbus_addr, 0x1900, 0x101009f);
        if (ret < 0)
            return ret;
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1900);
        /* printf("After 0x1900 0x%x\n", pbus_data); */
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x19a8);
        /* printf("Before 19a8 0x%x\n", pbus_data); */
        ret = airoha_pbus_write(mbus, pbus_addr, 0x19a8, pbus_data & ~BIT(16));
        if (ret < 0)
            return ret;
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x19a8);
        /* printf("After 19a8 0x%x\n", pbus_data); */
	}
    /* SMI ADDR */
    pbus_data = airoha_pbus_read(mbus, pbus_addr, EN8801S_RG_SMI_ADDR);
    pbus_data = (pbus_data & 0xffff0000) | (unsigned long)(EN8801S_PBUS_PHY_ID << 8) | (unsigned long)(EN8801S_MDIO_PHY_ID);
    ret = airoha_pbus_write(mbus, pbus_addr, EN8801S_RG_SMI_ADDR, pbus_data);
    pbus_addr = EN8801S_PBUS_PHY_ID;
    AIR_RTN_ERR(ret);
    mdelay(10);

    /* Optimze 10M IoT */
    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1690);
    pbus_data |= (1 << 31);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x1690, pbus_data);
    AIR_RTN_ERR(ret);
    /* set SGMII Base Page */
    ret = airoha_pbus_write(mbus, pbus_addr, 0x0600, 0x0c000c00);
    AIR_RTN_ERR(ret);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x10, 0xD801);
    AIR_RTN_ERR(ret);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x0,  0x9140);
    AIR_RTN_ERR(ret);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x0A14, 0x0003);
    AIR_RTN_ERR(ret);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x0600, 0x0c000c00);
    AIR_RTN_ERR(ret);
    /* Set FCM control */
    ret = airoha_pbus_write(mbus, pbus_addr, 0x1404, 0x004b);
    AIR_RTN_ERR(ret);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x140c, 0x0007);
    AIR_RTN_ERR(ret);

    ret = airoha_pbus_write(mbus, pbus_addr, 0x142c, 0x05050505);
    AIR_RTN_ERR(ret);
    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1440);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x1440, pbus_data & ~BIT(11));
    AIR_RTN_ERR(ret);

    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1408);
    ret = airoha_pbus_write(mbus, pbus_addr, 0x1408, pbus_data | BIT(5));
    AIR_RTN_ERR(ret);

    /* Set GPHY Perfomance*/
    /* Token Ring */
    ret = airoha_tr_reg_write(phydev, RgAddr_R1000DEC_15h, 0x0055A0);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_R1000DEC_17h, 0x07FF3F);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_PMA_00h,      0x00001E);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_PMA_01h,      0x6FB90A);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_PMA_17h,      0x060671);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_PMA_18h,      0x0E2F00);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_TR_26h,       0x444444);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_03h,     0x000000);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_06h,     0x2EBAEF);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_08h,     0x00000B);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_0Ch,     0x00504D);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_0Dh,     0x02314F);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_0Fh,     0x003028);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_10h,     0x005010);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_11h,     0x040001);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_13h,     0x018670);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_14h,     0x00024A);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_1Bh,     0x000072);
    AIR_RTN_ERR(ret);
    ret = airoha_tr_reg_write(phydev, RgAddr_DSPF_1Ch,     0x003210);
    AIR_RTN_ERR(ret);
    /* CL22 & CL45 */
    ret = airoha_cl22_write(mbus, phy_addr, 0x1f, 0x03);
    AIR_RTN_ERR(ret);
    GPHY_RG_LPI_1C.DATA = airoha_cl22_read(mbus, phy_addr, RgAddr_LPI_1Ch);
    if (GPHY_RG_LPI_1C.DATA < 0)
        return -EIO;
    GPHY_RG_LPI_1C.DataBitField.smi_deton_th = 0x0C;
    ret = airoha_cl22_write(mbus, phy_addr, RgAddr_LPI_1Ch, GPHY_RG_LPI_1C.DATA);
    AIR_RTN_ERR(ret);
    ret = airoha_cl22_write(mbus, phy_addr, RgAddr_LPI_1Ch, 0xC92);
    AIR_RTN_ERR(ret);
    ret = airoha_cl22_write(mbus, phy_addr, RgAddr_AUXILIARY_1Dh, 0x1);
    AIR_RTN_ERR(ret);
    ret = airoha_cl22_write(mbus, phy_addr, 0x1f, 0x0);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x120, 0x8014);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x122, 0xffff);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x123, 0xffff);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x144, 0x0200);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x14A, 0xEE20);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x189, 0x0110);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x19B, 0x0111);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x234, 0x0181);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x238, 0x0120);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x239, 0x0117);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x268, 0x07F4);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x2D1, 0x0733);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x323, 0x0011);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x324, 0x013F);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x326, 0x0037);
    AIR_RTN_ERR(ret);

    reg_value = airoha_cl45_read(phydev, 0x1E, 0x324);
    if (reg_value < 0)
        return -EIO;
    GPHY_RG_1E_324.DATA = (int)reg_value;
    GPHY_RG_1E_324.DataBitField.smi_det_deglitch_off = 0;
    ret = airoha_cl45_write(phydev, 0x1E, 0x324, GPHY_RG_1E_324.DATA);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x19E, 0xC2);
    AIR_RTN_ERR(ret);
    ret = airoha_cl45_write(phydev, 0x1E, 0x013, 0x0);
    AIR_RTN_ERR(ret);

    /* EFUSE */
    airoha_pbus_write(mbus, pbus_addr, 0x1C08, 0x40000040);
    retry = MAX_RETRY;
    while (0 != retry) {
        mdelay(1);
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1C08);
        if ((pbus_data & (1 << 30)) == 0) {
            break;
        }
        retry--;
    }
    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1C38);          /* RAW#2 */
    reg_value = airoha_cl45_read(phydev, 0x1E, 0x12);
    if (reg_value < 0)
        return -EIO;
    GPHY_RG_1E_012.DATA = reg_value;
    GPHY_RG_1E_012.DataBitField.da_tx_i2mpb_a_tbt = pbus_data & 0x03f;
    ret = airoha_cl45_write(phydev, 0x1E, 0x12, GPHY_RG_1E_012.DATA);
    AIR_RTN_ERR(ret);
    reg_value = airoha_cl45_read(phydev, 0x1E, 0x17);
    if (reg_value < 0)
        return -EIO;
    GPHY_RG_1E_017.DataBitField.da_tx_i2mpb_b_tbt = (reg_value >> 8) & 0x03f;
    ret = airoha_cl45_write(phydev, 0x1E, 0x17, GPHY_RG_1E_017.DATA);
    AIR_RTN_ERR(ret);

    ret = airoha_pbus_write(mbus, pbus_addr, 0x1C08, 0x40400040);
    AIR_RTN_ERR(ret);
    retry = MAX_RETRY;
    while (0 != retry) {
        mdelay(1);
        reg_value = airoha_pbus_read(mbus, pbus_addr, 0x1C08);
        if ((reg_value & (1 << 30)) == 0) {
            break;
        }
        retry--;
    }
    pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1C30);          /* RAW#16 */
    GPHY_RG_1E_324.DataBitField.smi_det_deglitch_off = (pbus_data >> 12) & 0x01;
    ret = airoha_cl45_write(phydev, 0x1E, 0x324, GPHY_RG_1E_324.DATA);
    AIR_RTN_ERR(ret);
#ifdef AIR_LED_SUPPORT
    ret = en8801s_led_init(phydev);
    if (ret != 0){
        printf("en8801s_led_init fail (ret:%d) !\n", ret);
    }
#endif
    printf("EN8801S initialize OK ! (%s)\n", EN8801S_DRIVER_VERSION);
    return 0;
}

int en8801s_read_status(struct phy_device *phydev)
{
    int ret, pbus_addr = EN8801S_PBUS_PHY_ID;
    struct mii_dev *mbus;
    unsigned long pbus_data;

    mbus = phydev->bus;
    if (SPEED_10 == phydev->speed) {
        /* set the bit for Optimze 10M IoT */
        debug("[Airoha] SPEED_10 0x1694\n");
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1694);
        pbus_data |= (1 << 31);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x1694, pbus_data);
        AIR_RTN_ERR(ret);
    } else {
        debug("[Airoha] SPEED_1000/100 0x1694\n");
        /* clear the bit for other speeds */
        pbus_data = airoha_pbus_read(mbus, pbus_addr, 0x1694);
        pbus_data &= ~(1 << 31);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x1694, pbus_data);
        AIR_RTN_ERR(ret);
    }

    airoha_pbus_write(mbus, pbus_addr, 0x0600, 0x0c000c00);
    if(SPEED_1000 == phydev->speed) {
        debug("[Airoha] SPEED_1000\n");
        ret = airoha_pbus_write(mbus, pbus_addr, 0x10, 0xD801);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0,  0x9140);
        AIR_RTN_ERR(ret);

        ret = airoha_pbus_write(mbus, pbus_addr, 0x0A14, 0x0003);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0600, 0x0c000c00);
        AIR_RTN_ERR(ret);
        mdelay(2);  /* delay 2 ms */
        ret = airoha_pbus_write(mbus, pbus_addr, 0x1404, 0x004b);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x140c, 0x0007);
        AIR_RTN_ERR(ret);
    }
    else if (SPEED_100 == phydev->speed) {
        debug("[Airoha] SPEED_100\n");
        ret = airoha_pbus_write(mbus, pbus_addr, 0x10, 0xD401);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0,  0x9140);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0A14, 0x0007);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0600, 0x0c11);
        AIR_RTN_ERR(ret);
        mdelay(2);  /* delay 2 ms */
        ret = airoha_pbus_write(mbus, pbus_addr, 0x1404, 0x0027);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x140c, 0x0007);
        AIR_RTN_ERR(ret);
    }
    else {
        debug("[Airoha] SPEED_10\n");
        ret = airoha_pbus_write(mbus, pbus_addr, 0x10, 0xD001);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0,  0x9140);
        AIR_RTN_ERR(ret);

        ret = airoha_pbus_write(mbus, pbus_addr, 0x0A14, 0x000b);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x0600, 0x0c11);
        AIR_RTN_ERR(ret);
         mdelay(2);  /* delay 2 ms */
        ret = airoha_pbus_write(mbus, pbus_addr, 0x1404, 0x0047);
        AIR_RTN_ERR(ret);
        ret = airoha_pbus_write(mbus, pbus_addr, 0x140c, 0x0007);
        AIR_RTN_ERR(ret);
    }
    return 0;
}

static int en8801s_startup(struct phy_device *phydev)
{
    int ret;

    ret = genphy_update_link(phydev);
    if (ret)
        return ret;
    ret = genphy_parse_link(phydev);
    if (ret)
        return ret;
    return en8801s_read_status(phydev);
}
#if AIR_UBOOT_REVISION > 0x202303
U_BOOT_PHY_DRIVER(en8801s) = {
    .name = "Airoha EN8801S",
    .uid = EN8801S_PHY_ID,
    .mask = 0x0ffffff0,
    .features = PHY_GBIT_FEATURES,
    .config = &en8801s_config,
    .startup = &en8801s_startup,
    .shutdown = &genphy_shutdown,
};
#else
static struct phy_driver AIR_EN8801S_driver = {
    .name = "Airoha EN8801S",
    .uid = EN8801S_PHY_ID,
    .mask = 0x0ffffff0,
    .features = PHY_GBIT_FEATURES,
    .config = &en8801s_config,
    .startup = &en8801s_startup,
    .shutdown = &genphy_shutdown,
};

int phy_air_en8801s_init(void)
{
    phy_register(&AIR_EN8801S_driver);
    return 0;
}
#endif