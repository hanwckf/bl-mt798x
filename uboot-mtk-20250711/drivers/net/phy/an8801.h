/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Airoha Technology Corp. (C) 2023
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("AIROHA SOFTWARE")
*  RECEIVED FROM AIROHA AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. AIROHA EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES AIROHA PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE AIROHA SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. AIROHA SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY AIROHA SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND AIROHA'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE AIROHA SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT AIROHA'S OPTION, TO REVISE OR REPLACE THE AIROHA SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  AIROHA FOR SUCH AIROHA SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*******************************************************************************/
/* SPDX-License-Identifier: GPL-2.0 */
/*************************************************
 * FILE NAME:  an8801.h
 * PURPOSE:
 *      AN8801SB PHY Driver for Uboot
 * NOTES:
 *
 *************************************************/

#ifndef __AN8801_H
#define __AN8801_H
/************************************************************************
*                  D E F I N E S
************************************************************************/
#define AIR_UBOOT_REVISION ((((U_BOOT_VERSION_NUM / 1000) % 10) << 20) | \
                        (((U_BOOT_VERSION_NUM / 100) % 10) << 16) | \
                        (((U_BOOT_VERSION_NUM / 10) % 10) << 12) | \
                        ((U_BOOT_VERSION_NUM % 10) << 8) | \
                        (((U_BOOT_VERSION_NUM_PATCH / 10) % 10) << 4) | \
                        ((U_BOOT_VERSION_NUM_PATCH % 10) << 0))
/* NAMING DECLARATIONS
 */

#define AN8801_DRIVER_VERSION  "1.0.1"

#define AN8801R_MDIO_PHY_ID     0x1
#define AN8801R_PHY_ID1         0xc0ff
#define AN8801R_PHY_ID2         0x0421
#define AN8801_PHY_ID     ((u32)((AN8801R_PHY_ID1 << 16) | AN8801R_PHY_ID2))

#define TRUE                    1
#define FALSE                   0
#define LINK_UP                 1
#define LINK_DOWN               0

#define MAX_LED_SIZE            3

#define MAX_RETRY               5

#define AN8801_EPHY_ADDR           0x11000000
#define AN8801_CL22                0x00800000

#define LED_ENABLE                  1
#define LED_DISABLE                 0

#ifndef BIT
#define BIT(nr)                     (1 << (nr))
#endif

/* chip control register */
#define AN8801SB_RG_GPIO_LED_INV        (0x10000010)
#define AN8801SB_RG_GPIO_DRIVING_E2     (0x10000018)
#define AN8801SB_RG_GPIO_LAN0_LED0_MODE (0x10000054)
#define AN8801SB_RG_GPIO_LAN0_LED0_SEL  (0x10000058)
#define AN8801SB_RG_FORCE_GPIO0_EN      (0x10000070)

#define LED_BCR                     (0x021)
#define LED_BCR_EXT_CTRL            BIT(15)
#define LED_BCR_EVT_ALL             BIT(4)
#define LED_BCR_CLK_EN              BIT(3)
#define LED_BCR_TIME_TEST           BIT(2)
#define LED_BCR_MODE_MASK           (3)
#define LED_BCR_MODE_DISABLE        (0)
#define LED_BCR_MODE_2LED           (1)
#define LED_BCR_MODE_3LED_1         (2)
#define LED_BCR_MODE_3LED_2         (3)

#define LED_ON_DUR                  (0x022)
#define LED_ON_DUR_MASK             (0xffff)

#define LED_BLK_DUR                 (0x023)
#define LED_BLK_DUR_MASK            (0xffff)

#define LED_ON_CTRL(i)              (0x024 + ((i) * 2))
#define LED_ON_EN                   BIT(15)
#define LED_ON_POL                  BIT(14)
#define LED_ON_EVT_MASK             (0x7f)
#define LED_ON_EVT_FORCE            BIT(6)
#define LED_ON_EVT_HDX              BIT(5)
#define LED_ON_EVT_FDX              BIT(4)
#define LED_ON_EVT_LINK_DN          BIT(3)
#define LED_ON_EVT_LINK_10M         BIT(2)
#define LED_ON_EVT_LINK_100M        BIT(1)
#define LED_ON_EVT_LINK_1000M       BIT(0)

#define LED_BLK_CTRL(i)             (0x025 + ((i) * 2))
#define LED_BLK_EVT_MASK            (0x3ff)
#define LED_BLK_EVT_FORCE           BIT(9)
#define LED_BLK_EVT_10M_RX          BIT(5)
#define LED_BLK_EVT_10M_TX          BIT(4)
#define LED_BLK_EVT_100M_RX         BIT(3)
#define LED_BLK_EVT_100M_TX         BIT(2)
#define LED_BLK_EVT_1000M_RX        BIT(1)
#define LED_BLK_EVT_1000M_TX        BIT(0)

#define UNIT_LED_BLINK_DURATION     1024

#define AN8801SB_SGMII_AN0              (0x10220000)
#define AN8801SB_SGMII_AN4              (0x10220010)
#define AN8801SB_PCS_CTRL1              (0x10220a00)
#define AN8801SB_EFIFO_MODE             (0x10270100)
#define AN8801SB_EFIFO_CTRL             (0x10270104)
#define AN8801SB_EFIFO_WM               (0x10270108)
#define SGMII_PCS_FORCE_SYNC_OFF        (0x2)
#define SGMII_PCS_FORCE_SYNC_MASK       (0x6)
#define EFIFO_MODE_1000                 (0x0f)
#define EFIFO_MODE_10_100               (0x0c)
#define EFIFO_CTRL_1000                 (0x3f)
#define EFIFO_CTRL_10_100               (0xff)
#define EFIFO_WM_VALUE_1G               (0x10100303)
#define EFIFO_WM_VALUE_10_100           (0x05050303)
#define AN8801SB_SGMII_AN0_ANRESTART    (0x0200)    /* Serdes auto negotation restart */

#define PHY_PRE_SPEED_REG               (0x2b)

#define MMD_DEV_VSPEC2          (0x1F)

#define RGMII_DELAY_STEP_MASK       0x7
#define RGMII_RXDELAY_ALIGN         BIT(4)
#define RGMII_RXDELAY_FORCE_MODE    BIT(24)
#define RGMII_TXDELAY_FORCE_MODE    BIT(24)

/*
For reference only
*/
/* User-defined.B */
/* Link on(1G/100M/10M), no activity */
#define AIR_LED0_ON \
    (LED_ON_EVT_LINK_1000M | LED_ON_EVT_LINK_100M | LED_ON_EVT_LINK_10M)
#define AIR_LED0_BLK     (0x0)
/* No link on, activity(1G/100M/10M TX/RX) */
#define AIR_LED1_ON      (0x0)
#define AIR_LED1_BLK \
    (LED_BLK_EVT_1000M_TX | LED_BLK_EVT_1000M_RX | \
    LED_BLK_EVT_100M_TX | LED_BLK_EVT_100M_RX | \
    LED_BLK_EVT_10M_TX | LED_BLK_EVT_10M_RX)
/* Link on(100M/10M), activity(100M/10M TX/RX) */
#define AIR_LED2_ON      (LED_ON_EVT_LINK_100M | LED_ON_EVT_LINK_10M)
#define AIR_LED2_BLK \
    (LED_BLK_EVT_100M_TX | LED_BLK_EVT_100M_RX | \
    LED_BLK_EVT_10M_TX | LED_BLK_EVT_10M_RX)
/* User-defined.E */

/* Invalid data */
#define INVALID_DATA            0xffffffff

#define LED_BLINK_DURATION(f)       (UNIT_LED_BLINK_DURATION << (f))
#define LED_GPIO_SEL(led, gpio)     ((led) << ((gpio) * 3))

/* DATA TYPE DECLARATIONS
 */
enum AIR_LED_GPIO_PIN_T {
    AIR_LED_GPIO1 = 1,
    AIR_LED_GPIO2,
    AIR_LED_GPIO3,
    AIR_LED_GPIO5 = 5,
    AIR_LED_GPIO8 = 8,
    AIR_LED_GPIO9
};

enum AIR_LED_T {
    AIR_LED0 = 0,
    AIR_LED1,
    AIR_LED2,
    AIR_LED3
};

enum AIR_LED_BLK_DUT_T {
    AIR_LED_BLK_DUR_32M = 0,
    AIR_LED_BLK_DUR_64M,
    AIR_LED_BLK_DUR_128M,
    AIR_LED_BLK_DUR_256M,
    AIR_LED_BLK_DUR_512M,
    AIR_LED_BLK_DUR_1024M,
    AIR_LED_BLK_DUR_LAST
};

enum AIR_LED_POLARITY {
    AIR_ACTIVE_LOW = 0,
    AIR_ACTIVE_HIGH,
};

enum AIR_LED_MODE_T {
    AIR_LED_MODE_DISABLE = 0,
    AIR_LED_MODE_USER_DEFINE,
    AIR_LED_MODE_LAST
};

enum AIR_RGMII_DELAY_STEP_T {
    AIR_RGMII_DELAY_NOSTEP = 0,
    AIR_RGMII_DELAY_STEP_1 = 1,
    AIR_RGMII_DELAY_STEP_2 = 2,
    AIR_RGMII_DELAY_STEP_3 = 3,
    AIR_RGMII_DELAY_STEP_4 = 4,
    AIR_RGMII_DELAY_STEP_5 = 5,
    AIR_RGMII_DELAY_STEP_6 = 6,
    AIR_RGMII_DELAY_STEP_7 = 7,
};

struct AIR_LED_CFG_T {
    u16 en;
    u16 gpio;
    u16 pol;
    u16 on_cfg;
    u16 blk_cfg;
};

struct an8801r_priv {
    struct AIR_LED_CFG_T  led_cfg[MAX_LED_SIZE];
    u32                   led_blink_cfg;
    u8                    rxdelay_force;
    u8                    txdelay_force;
    u16                   rxdelay_step;
    u8                    rxdelay_align;
    u16                   txdelay_step;
};

#endif /* End of __AN8801_H */

