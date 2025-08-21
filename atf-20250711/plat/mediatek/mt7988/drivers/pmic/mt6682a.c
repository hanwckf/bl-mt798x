/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2022, MediaTek Inc. All rights reserved.
 *
 * Author: Sen Chu <sen.chu@mediatek.com>
 */


#include <assert.h>
#include <common/debug.h>
#include "mt_i2c.h"
#include "mt6682a.h"

static struct regulator_info mt6682a_info[REGULATOR_MAX_NR] = {
	[REGULATOR_BUCK1] = mt6682a_decl(buck1),	// 5V output
	[REGULATOR_BUCK2] = mt6682a_decl(buck2),	// VCORE and VSRAM_CORE
	[REGULATOR_BUCK3] = mt6682a_decl(buck3),	// VPROC and VSRAM_PROC
	[REGULATOR_BUCK4] = mt6682a_decl(buck4),	// IO 1.8V and others
	[REGULATOR_LDO] = mt6682a_decl(ldo),		// USB/PCIe/Ethernet 1.8V
};

int mt6682a_set_voltage(uint8_t id, int volt, int max_volt)
{
    unsigned short selector = 0;
    int ret = 0;

    if (volt > mt6682a_info[id].max_uV || volt < mt6682a_info[id].min_uV) {
        ERROR("Invalid input voltage\n");
        return -1;
    }

    selector = DIV_ROUND_UP((volt - mt6682a_info[id].min_uV),
						mt6682a_info[id].step_uV);
	selector &= mt6682a_info[id].vol_mask;

    ret = mtk_i2c_write_reg(MT6682A_I2C_SLAVE, mt6682a_info[id].vol_reg,
						1, selector);

    return ret;
}

int mt6682a_get_voltage(uint8_t id)
{
    int ret = 0;
    uint8_t val = 0;
    int volt;

    ret = mtk_i2c_read_reg(MT6682A_I2C_SLAVE, mt6682a_info[id].vol_reg, 1, &val);
    if (ret < 0) {
        ERROR("Get voltage register fail, ret = %d!\n", ret);
        return ret;
    }
	val &= mt6682a_info[id].vol_mask;
    volt = BUCK_VAL_TO_VOLT(val);

    return volt;
}

int mt6682a_enable(uint8_t id, uint8_t en)
{
    int ret = 0;
	uint8_t val = 0;

    if (mt6682a_info[id].enable_reg == 0)
        return -1;

    NOTICE("id = %d, en = %d\n", id, en);

	ret = mtk_i2c_read_reg(MT6682A_I2C_SLAVE, mt6682a_info[id].enable_reg, 1, &val);
	if (ret < 0) {
        ERROR("Get enable register fail, ret = %d!\n", ret);
        return ret;
    }
	val &= (~(1 << mt6682a_info[id].enable_shift));
	val |= (en << mt6682a_info[id].enable_shift);
    ret = mtk_i2c_write_reg(MT6682A_I2C_SLAVE, mt6682a_info[id].enable_reg, 1, val);

    return ret;
}

int mt6682a_set_mode(uint8_t id, uint8_t mode)
{
    int ret = 0;
	uint8_t val = 0;

    if (mt6682a_info[id].mode_reg== 0)
        return -1;

    NOTICE("id = %d, mode = %d\n", id, mode);

	ret = mtk_i2c_read_reg(MT6682A_I2C_SLAVE, mt6682a_info[id].mode_reg, 1, &val);
	if (ret < 0) {
        ERROR("Get enable register fail, ret = %d!\n", ret);
        return ret;
    }
	val &= (~(1 << mt6682a_info[id].mode_shift));
	val |= (mode << mt6682a_info[id].mode_shift);
    ret = mtk_i2c_write_reg(MT6682A_I2C_SLAVE, mt6682a_info[id].enable_reg, 1, val);

    return ret;

}

static void MT6682A_INIT_SETTING_V1(void)
{
}

//==============================================================================
// PMIC MT6682A Init Code
//==============================================================================
void mt6682a_init(void)
{
	uint8_t val = 0;

    VERBOSE("[bl2] mt6682a init start......\n");

    MT6682A_INIT_SETTING_V1();
	mtk_i2c_read_reg(MT6682A_I2C_SLAVE, MT6682A_REG_PGOOD_STATUS, 1, &val);
	VERBOSE("PGOOD Status = 0x%x\n", val);

    VERBOSE("[bl2] mt6682a init done.\n");
}
