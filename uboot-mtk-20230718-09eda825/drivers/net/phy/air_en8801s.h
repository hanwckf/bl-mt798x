/* SPDX-License-Identifier: GPL-2.0 */
/*************************************************
 * FILE NAME:  air_en8801s.h
 * PURPOSE:
 *      EN8801S PHY Driver for Uboot
 * NOTES:
 *
 *  Copyright (C) 2023 Airoha Technology Corp.
 *************************************************/

#ifndef __EN8801S_H
#define __EN8801S_H

/************************************************************************
*                  D E F I N E S
************************************************************************/
#define AIR_UBOOT_REVISION ((((U_BOOT_VERSION_NUM / 1000) % 10) << 20) | \
		      (((U_BOOT_VERSION_NUM / 100) % 10) << 16) | \
		      (((U_BOOT_VERSION_NUM / 10) % 10) << 12) | \
		      ((U_BOOT_VERSION_NUM % 10) << 8) | \
		      (((U_BOOT_VERSION_NUM_PATCH / 10) % 10) << 4) | \
		      ((U_BOOT_VERSION_NUM_PATCH % 10) << 0))

#define EN8801S_MDIO_DEFAULT_ID 0x1d
#define EN8801S_PBUS_DEFAULT_ID (EN8801S_MDIO_DEFAULT_ID + 1)
#define EN8801S_MDIO_PHY_ID     0x18       /* Range PHY_ADDRESS_RANGE .. 0x1e */
#define EN8801S_PBUS_PHY_ID     (EN8801S_MDIO_PHY_ID + 1)
#define EN8801S_DRIVER_VERSION      "v1.1.3"

#define EN8801S_RG_ETHER_PHY_OUI    0x19a4
#define EN8801S_RG_SMI_ADDR         0x19a8
#define EN8801S_PBUS_OUI            0x17a5
#define EN8801S_RG_BUCK_CTL         0x1a20
#define EN8801S_RG_LTR_CTL          0x0cf8

#define EN8801S_PHY_ID1         0x03a2
#define EN8801S_PHY_ID2         0x9461
#define EN8801S_PHY_ID          (unsigned long)((EN8801S_PHY_ID1 << 16) | EN8801S_PHY_ID2)

/*
SFP Sample for verification
Tx Reverse, Rx Reverse
*/
#define EN8801S_TX_POLARITY_NORMAL   0x0
#define EN8801S_TX_POLARITY_REVERSE  0x1

#define EN8801S_RX_POLARITY_NORMAL   (0x1 << 1)
#define EN8801S_RX_POLARITY_REVERSE  (0x0 << 1)

#ifndef BIT
#define BIT(nr)			(1UL << (nr))
#endif

#define MAX_RETRY               5
#define MAX_OUI_CHECK           2

/* CL45 MDIO control */
#define MII_MMD_ACC_CTL_REG     0x0d
#define MII_MMD_ADDR_DATA_REG   0x0e
#define MMD_OP_MODE_DATA        BIT(14)

#define MAX_TRG_COUNTER         5

/* TokenRing Reg Access */
#define TrReg_PKT_XMT_STA       0x8000
#define TrReg_WR                0x8000
#define TrReg_RD                0xA000

#define RgAddr_LPI_1Ch       0x1c
#define RgAddr_AUXILIARY_1Dh 0x1d
#define RgAddr_PMA_00h       0x0f80
#define RgAddr_PMA_01h       0x0f82
#define RgAddr_PMA_17h       0x0fae
#define RgAddr_PMA_18h       0x0fb0
#define RgAddr_DSPF_03h      0x1686
#define RgAddr_DSPF_06h      0x168c
#define RgAddr_DSPF_08h      0x1690
#define RgAddr_DSPF_0Ch      0x1698
#define RgAddr_DSPF_0Dh      0x169a
#define RgAddr_DSPF_0Fh      0x169e
#define RgAddr_DSPF_10h      0x16a0
#define RgAddr_DSPF_11h      0x16a2
#define RgAddr_DSPF_13h      0x16a6
#define RgAddr_DSPF_14h      0x16a8
#define RgAddr_DSPF_1Bh      0x16b6
#define RgAddr_DSPF_1Ch      0x16b8
#define RgAddr_TR_26h        0x0ecc
#define RgAddr_R1000DEC_15h  0x03aa
#define RgAddr_R1000DEC_17h  0x03ae

/*
The following led_cfg example is for reference only.
LED5 1000M/LINK/ACT  (GPIO5)  <-> BASE_T_LED0,
LED6 10/100M/LINK/ACT(GPIO9)  <-> BASE_T_LED1,
LED4 100M/LINK/ACT   (GPIO8)  <-> BASE_T_LED2,
*/
/* User-defined.B */
#define BASE_T_LED0_ON_CFG      (LED_ON_EVT_LINK_1000M)
#define BASE_T_LED0_BLK_CFG     (LED_BLK_EVT_1000M_TX_ACT | LED_BLK_EVT_1000M_RX_ACT)
#define BASE_T_LED1_ON_CFG      (LED_ON_EVT_LINK_100M | LED_ON_EVT_LINK_10M)
#define BASE_T_LED1_BLK_CFG     (LED_BLK_EVT_100M_TX_ACT | LED_BLK_EVT_100M_RX_ACT | \
                                 LED_BLK_EVT_10M_TX_ACT | LED_BLK_EVT_10M_RX_ACT )
#define BASE_T_LED2_ON_CFG      (LED_ON_EVT_LINK_100M)
#define BASE_T_LED2_BLK_CFG     (LED_BLK_EVT_100M_TX_ACT | LED_BLK_EVT_100M_RX_ACT)
#define BASE_T_LED3_ON_CFG      (0x0)
#define BASE_T_LED3_BLK_CFG     (0x0)
/* User-defined.E */

#define EN8801S_LED_COUNT       4

#define LED_BCR                     (0x021)
#define LED_BCR_EXT_CTRL            (1 << 15)
#define LED_BCR_CLK_EN              (1 << 3)
#define LED_BCR_TIME_TEST           (1 << 2)
#define LED_BCR_MODE_MASK           (3)
#define LED_BCR_MODE_DISABLE        (0)
#define LED_ON_CTRL(i)              (0x024 + ((i)*2))
#define LED_ON_EN                   (1 << 15)
#define LED_ON_POL                  (1 << 14)
#define LED_ON_EVT_MASK             (0x7f)
/* LED ON Event Option.B */
#define LED_ON_EVT_FORCE            (1 << 6)
#define LED_ON_EVT_LINK_DOWN        (1 << 3)
#define LED_ON_EVT_LINK_10M         (1 << 2)
#define LED_ON_EVT_LINK_100M        (1 << 1)
#define LED_ON_EVT_LINK_1000M       (1 << 0)
/* LED ON Event Option.E */
#define LED_BLK_CTRL(i)             (0x025 + ((i)*2))
#define LED_BLK_EVT_MASK            (0x3ff)
/* LED Blinking Event Option.B*/
#define LED_BLK_EVT_FORCE           (1 << 9)
#define LED_BLK_EVT_10M_RX_ACT      (1 << 5)
#define LED_BLK_EVT_10M_TX_ACT      (1 << 4)
#define LED_BLK_EVT_100M_RX_ACT     (1 << 3)
#define LED_BLK_EVT_100M_TX_ACT     (1 << 2)
#define LED_BLK_EVT_1000M_RX_ACT    (1 << 1)
#define LED_BLK_EVT_1000M_TX_ACT    (1 << 0)
/* LED Blinking Event Option.E*/
#define LED_ON_DUR                  (0x022)
#define LED_ON_DUR_MASK             (0xffff)
#define LED_BLK_DUR                 (0x023)
#define LED_BLK_DUR_MASK            (0xffff)

#define LED_ENABLE                  1
#define LED_DISABLE                 0

#define UNIT_LED_BLINK_DURATION     1024

#define AIR_RTN_ON_ERR(cond, err)  \
    do { if ((cond)) return (err); } while(0)

#define AIR_RTN_ERR(err)                       AIR_RTN_ON_ERR(err < 0, err)

#define LED_SET_EVT(reg, cod, result, bit) do         \
    {                                                 \
        if(reg & cod) {                               \
            result |= bit;                            \
        }                                             \
    } while(0)

#define LED_SET_GPIO_SEL(gpio, led, val) do           \
    {                                                 \
            val |= (led << (8 * (gpio % 4)));         \
    } while(0)

/* DATA TYPE DECLARATIONS
 */
typedef struct
{
    int DATA_Lo;
    int DATA_Hi;
}TR_DATA_T;

typedef union
{
    struct
    {
        /* b[15:00] */
        int smi_deton_wt                             : 3;
        int smi_det_mdi_inv                          : 1;
        int smi_detoff_wt                            : 3;
        int smi_sigdet_debouncing_en                 : 1;
        int smi_deton_th                             : 6;
        int rsv_14                                   : 2;
    } DataBitField;
    int DATA;
} gephy_all_REG_LpiReg1Ch, *Pgephy_all_REG_LpiReg1Ch;

typedef union
{
    struct
    {
        /* b[15:00] */
        int rg_smi_detcnt_max                        : 6;
        int rsv_6                                    : 2;
        int rg_smi_det_max_en                        : 1;
        int smi_det_deglitch_off                     : 1;
        int rsv_10                                   : 6;
    } DataBitField;
    int DATA;
} gephy_all_REG_dev1Eh_reg324h, *Pgephy_all_REG_dev1Eh_reg324h;

typedef union
{
    struct
    {
        /* b[15:00] */
        int da_tx_i2mpb_a_tbt                        : 6;
        int rsv_6                                    : 4;
        int da_tx_i2mpb_a_gbe                        : 6;
    } DataBitField;
    int DATA;
} gephy_all_REG_dev1Eh_reg012h, *Pgephy_all_REG_dev1Eh_reg012h;

typedef union
{
    struct
    {
        /* b[15:00] */
        int da_tx_i2mpb_b_tbt                        : 6;
        int rsv_6                                    : 2;
        int da_tx_i2mpb_b_gbe                        : 6;
        int rsv_14                                   : 2;
    } DataBitField;
    int DATA;
} gephy_all_REG_dev1Eh_reg017h, *Pgephy_all_REG_dev1Eh_reg017h;

typedef struct AIR_BASE_T_LED_CFG_S
{
    u16 en;
    u16 gpio;
    u16 pol;
    u16 on_cfg;
    u16 blk_cfg;
}AIR_BASE_T_LED_CFG_T;

typedef enum
{
    AIR_LED_BLK_DUR_32M,
    AIR_LED_BLK_DUR_64M,
    AIR_LED_BLK_DUR_128M,
    AIR_LED_BLK_DUR_256M,
    AIR_LED_BLK_DUR_512M,
    AIR_LED_BLK_DUR_1024M,
    AIR_LED_BLK_DUR_LAST
} AIR_LED_BLK_DUT_T;

typedef enum
{
    AIR_ACTIVE_LOW,
    AIR_ACTIVE_HIGH,
} AIR_LED_POLARITY;
typedef enum
{
    AIR_LED_MODE_DISABLE,
    AIR_LED_MODE_USER_DEFINE,
    AIR_LED_MODE_LAST
} AIR_LED_MODE_T;

/************************************************************************
*                  F U N C T I O N    P R O T O T Y P E S
************************************************************************/

unsigned long airoha_pbus_read(struct mii_dev *bus, int pbus_addr, int pbus_reg);
int airoha_pbus_write(struct mii_dev *bus, int pbus_addr, int pbus_reg, unsigned long pbus_data);
int airoha_phy_process(void);
#endif /* __AIROHA_H */
