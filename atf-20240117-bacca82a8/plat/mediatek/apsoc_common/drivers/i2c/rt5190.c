#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/gpio.h>
#include "mt_i2c.h"
#include "rt5190.h"

#define PMIC_I2C_SLAVE	       0x64
#define PMIC_I2C_SLAVE_ADDRLEN 1

#define RT5190_REG_BUCK2_SEL 0x4
#define RT5190_REG_BUCK3_SEL 0x5
#define RT5190_REG_ENABLE    0x7

#define RT5190_LDO_EN	BIT(5)
#define RT5190_BUCK4_EN BIT(4)
#define RT5190_BUCK3_EN BIT(3)
#define RT5190_BUCK2_EN BIT(2)
#define RT5190_BUCK1_EN BIT(1)

int reg_to_mv(uint8_t reg)
{
	int val = ((reg & 0x7f) * 10) + 600;

	if (val > 1400)
		return 1400;

	return val;
}

uint8_t mv_to_reg(int mv)
{
	uint8_t val = (mv - 600) / 10;

	if (val > 0x5f)
		return 0x5f;

	return (val & 0x7f);
}

int rt5190_pmic_test(void)
{
	uint8_t chip = PMIC_I2C_SLAVE;
	int addrlen = PMIC_I2C_SLAVE_ADDRLEN;
	uint8_t val = 0x19;
	int buck2_mv, buck3_mv;

	mtk_i2c_read_reg(chip, RT5190_REG_BUCK2_SEL, addrlen, &val);
	buck2_mv = reg_to_mv(val);
	mtk_i2c_read_reg(chip, RT5190_REG_BUCK3_SEL, addrlen, &val);
	buck3_mv = reg_to_mv(val);
	VERBOSE("PMIC: CURRENT BUCK2=%d(mv) BUCK3=%d(mv)\n", buck2_mv, buck3_mv);
	buck2_mv = 650;
	buck3_mv = 850;
	VERBOSE("PMIC: CONFIG BUCK2=%d(mv) BUCK3=%d(mv)\n", buck2_mv, buck3_mv);
	mtk_i2c_write_reg(chip, RT5190_REG_BUCK2_SEL, addrlen,
			  mv_to_reg(buck2_mv));
	mtk_i2c_write_reg(chip, RT5190_REG_BUCK3_SEL, addrlen,
			  mv_to_reg(buck3_mv));
	mtk_i2c_read_reg(chip, RT5190_REG_BUCK2_SEL, addrlen, &val);
	buck2_mv = reg_to_mv(val);
	mtk_i2c_read_reg(chip, RT5190_REG_BUCK3_SEL, addrlen, &val);
	buck3_mv = reg_to_mv(val);
	VERBOSE("PMIC: CURRENT BUCK2=%d(mv) BUCK3=%d(mv)\n", buck2_mv, buck3_mv);

	return 0;
}
