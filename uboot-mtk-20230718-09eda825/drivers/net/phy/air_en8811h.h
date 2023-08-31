/* SPDX-License-Identifier: GPL-2.0 */
/*************************************************
 * FILE NAME:  air_en8811h.h
 * PURPOSE:
 *      EN8811H PHY Driver for Uboot
 * NOTES:
 *
 *  Copyright (C) 2023 Airoha Technology Corp.
 *************************************************/

#ifndef __EN8811H_H
#define __EN8811H_H

#define AIR_UBOOT_REVISION ((((U_BOOT_VERSION_NUM / 1000) % 10) << 20) | \
			(((U_BOOT_VERSION_NUM / 100) % 10) << 16) | \
			(((U_BOOT_VERSION_NUM / 10) % 10) << 12) | \
			((U_BOOT_VERSION_NUM % 10) << 8) | \
			(((U_BOOT_VERSION_NUM_PATCH / 10) % 10) << 4) | \
			((U_BOOT_VERSION_NUM_PATCH % 10) << 0))

#define EN8811H_PHY_ID1             0x03a2
#define EN8811H_PHY_ID2             0xa411
#define EN8811H_PHY_ID              ((EN8811H_PHY_ID1 << 16) | EN8811H_PHY_ID2)
#define EN8811H_SPEED_2500          0x03
#define EN8811H_PHY_READY           0x02
#define MAX_RETRY                   5

#define EN8811H_TX_POLARITY_NORMAL   0x1
#define EN8811H_TX_POLARITY_REVERSE  0x0

#define EN8811H_RX_POLARITY_NORMAL  (0x0 << 1)
#define EN8811H_RX_POLARITY_REVERSE (0x1 << 1)

#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

/* CL45 MDIO control */
#define MII_MMD_ACC_CTL_REG         0x0d
#define MII_MMD_ADDR_DATA_REG       0x0e
#define MMD_OP_MODE_DATA            BIT(14)
/* MultiGBASE-T AN register */
#define MULTIG_ANAR_2500M           (0x0080)
#define MULTIG_LPAR_2500M           (0x0020)

#define EN8811H_DRIVER_VERSION  "v1.0.4"

/************************************************************
 * For reference only
 * LED0 Link 2500/Blink 2500 TxRx (GPIO5)    <-> BASE_T_LED0,
 * LED1 Link 1000/Blink 1000 TxRx (GPIO4)    <-> BASE_T_LED1,
 * LED2 Link 100/Blink 100 TxRx   (GPIO3)    <-> BASE_T_LED2,
 ************************************************************/
/* User-defined.B */
#define AIR_LED0_ON      (LED_ON_EVT_LINK_2500M)
#define AIR_LED0_BLK     (LED_BLK_EVT_2500M_TX_ACT | LED_BLK_EVT_2500M_RX_ACT)
#define AIR_LED1_ON      (LED_ON_EVT_LINK_1000M)
#define AIR_LED1_BLK    (LED_BLK_EVT_1000M_TX_ACT | LED_BLK_EVT_1000M_RX_ACT)
#define AIR_LED2_ON      (LED_ON_EVT_LINK_100M)
#define AIR_LED2_BLK     (LED_BLK_EVT_100M_TX_ACT | LED_BLK_EVT_100M_RX_ACT)
/* User-defined.E */

#define LED_ON_CTRL(i)              (0x024 + ((i)*2))
#define LED_ON_EN                   (1 << 15)
#define LED_ON_POL                  (1 << 14)
#define LED_ON_EVT_MASK             (0x1ff)
/* LED ON Event Option.B */
#define LED_ON_EVT_LINK_2500M       (1 << 8)
#define LED_ON_EVT_FORCE            (1 << 6)
#define LED_ON_EVT_HDX              (1 << 5)
#define LED_ON_EVT_FDX              (1 << 4)
#define LED_ON_EVT_LINK_DOWN        (1 << 3)
#define LED_ON_EVT_LINK_100M        (1 << 1)
#define LED_ON_EVT_LINK_1000M       (1 << 0)
/* LED ON Event Option.E */

#define LED_BLK_CTRL(i)             (0x025 + ((i)*2))
#define LED_BLK_EVT_MASK            (0xfff)
/* LED Blinking Event Option.B*/
#define LED_BLK_EVT_2500M_RX_ACT    (1 << 11)
#define LED_BLK_EVT_2500M_TX_ACT    (1 << 10)
#define LED_BLK_EVT_FORCE           (1 << 9)
#define LED_BLK_EVT_100M_RX_ACT     (1 << 3)
#define LED_BLK_EVT_100M_TX_ACT     (1 << 2)
#define LED_BLK_EVT_1000M_RX_ACT    (1 << 1)
#define LED_BLK_EVT_1000M_TX_ACT    (1 << 0)
/* LED Blinking Event Option.E*/
#define LED_ENABLE                  1
#define LED_DISABLE                 0

#define EN8811H_LED_COUNT       3

#define LED_BCR                     (0x021)
#define LED_BCR_EXT_CTRL            (1 << 15)
#define LED_BCR_CLK_EN              (1 << 3)
#define LED_BCR_TIME_TEST           (1 << 2)
#define LED_BCR_MODE_MASK           (3)
#define LED_BCR_MODE_DISABLE        (0)
#define LED_BCR_MODE_2LED           (1)
#define LED_BCR_MODE_3LED_1         (2)
#define LED_BCR_MODE_3LED_2         (3)

#define LED_ON_DUR                  (0x022)
#define LED_ON_DUR_MASK             (0xffff)

#define LED_BLK_DUR                 (0x023)
#define LED_BLK_DUR_MASK            (0xffff)

#define LED_GPIO_SEL_MASK           0x7FFFFFF

#define UNIT_LED_BLINK_DURATION     1024

#define INVALID_DATA                0xffff
#define PBUS_INVALID_DATA           0xffffffff

struct air_base_t_led_cfg_s {
    u16 en;
    u16 gpio;
    u16 pol;
    u16 on_cfg;
    u16 blk_cfg;
};

enum {
    AIR_LED2_GPIO3 = 3,
    AIR_LED1_GPIO4,
    AIR_LED0_GPIO5,
    AIR_LED_LAST
};

enum {
    AIR_BASE_T_LED0,
    AIR_BASE_T_LED1,
    AIR_BASE_T_LED2,
    AIR_BASE_T_LED3
};

enum {
    AIR_LED_BLK_DUR_32M,
    AIR_LED_BLK_DUR_64M,
    AIR_LED_BLK_DUR_128M,
    AIR_LED_BLK_DUR_256M,
    AIR_LED_BLK_DUR_512M,
    AIR_LED_BLK_DUR_1024M,
    AIR_LED_BLK_DUR_LAST
};

enum {
    AIR_ACTIVE_LOW,
    AIR_ACTIVE_HIGH,
};

enum {
    AIR_LED_MODE_DISABLE,
    AIR_LED_MODE_USER_DEFINE,
    AIR_LED_MODE_LAST
};

#endif /* End of __EN8811H_MD32_H */

