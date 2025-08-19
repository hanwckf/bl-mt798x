/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef PMIC_H
#define PMIC_H

/* PMIC Registers */
#define MT6380_ALDO_CON_0                         0x0000
#define MT6380_BTLDO_CON_0                        0x0004
#define MT6380_COMP_CON_0                         0x0008
#define MT6380_CPUBUCK_CON_0                      0x000C
#define MT6380_CPUBUCK_CON_1                      0x0010
#define MT6380_CPUBUCK_CON_2                      0x0014
#define MT6380_DDRLDO_CON_0                       0x0018
#define MT6380_MLDO_CON_0                         0x001C
#define MT6380_PALDO_CON_0                        0x0020
#define MT6380_PHYLDO_CON_0                       0x0024
#define MT6380_SIDO_CON_0                         0x0028
#define MT6380_SIDO_CON_1                         0x002C
#define MT6380_SIDO_CON_2                         0x0030
#define MT6380_SLDO_CON_0                         0x0034
#define MT6380_TLDO_CON_0                         0x0038
#define MT6380_STARTUP_CON_0                      0x003C
#define MT6380_STARTUP_CON_1                      0x0040
#define MT6380_SMPS_TOP_CON_0                     0x0044
#define MT6380_SMPS_TOP_CON_1                     0x0048
#define MT6380_ANA_CTRL_0                         0x0050
#define MT6380_ANA_CTRL_1                         0x0054
#define MT6380_ANA_CTRL_2                         0x0058
#define MT6380_ANA_CTRL_3                         0x005C
#define MT6380_ANA_CTRL_4                         0x0060
#define MT6380_SPK_CON9                           0x0064
#define MT6380_SPK_CON11                          0x0068
#define MT6380_SPK_CON12                          0x006A
#define MT6380_CLK_CTRL                           0x0070
#define MT6380_PINMUX_CTRL                        0x0074
#define MT6380_IO_CTRL                            0x0078
#define MT6380_SLP_MODE_CTRL_0                    0x007C
#define MT6380_SLP_MODE_CTRL_1                    0x0080
#define MT6380_SLP_MODE_CTRL_2                    0x0084
#define MT6380_SLP_MODE_CTRL_3                    0x0088
#define MT6380_SLP_MODE_CTRL_4                    0x008C
#define MT6380_SLP_MODE_CTRL_5                    0x0090
#define MT6380_SLP_MODE_CTRL_6                    0x0094
#define MT6380_SLP_MODE_CTRL_7                    0x0098
#define MT6380_SLP_MODE_CTRL_8                    0x009C
#define MT6380_FCAL_CTRL_0                        0x00A0
#define MT6380_FCAL_CTRL_1                        0x00A4
#define MT6380_LDO_CTRL_0                         0x00A8
#define MT6380_LDO_CTRL_1                         0x00AC
#define MT6380_LDO_CTRL_2                         0x00B0
#define MT6380_LDO_CTRL_3                         0x00B4
#define MT6380_LDO_CTRL_4                         0x00B8
#define MT6380_DEBUG_CTRL_0                       0x00BC
#define MT6380_EFU_CTRL_0                         0x0200
#define MT6380_EFU_CTRL_1                         0x0201
#define MT6380_EFU_CTRL_2                         0x0202
#define MT6380_EFU_CTRL_3                         0x0203
#define MT6380_EFU_CTRL_4                         0x0204
#define MT6380_EFU_CTRL_5                         0x0205
#define MT6380_EFU_CTRL_6                         0x0206
#define MT6380_EFU_CTRL_7                         0x0207
#define MT6380_EFU_CTRL_8                         0x0208

void mtk_pmic_init(void);

#endif /* PMIC_H */
