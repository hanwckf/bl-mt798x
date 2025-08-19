#ifndef MT6682A_H
#define MT6682A_H

#include <stdint.h>

#define DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))
#define BUCK_VAL_TO_VOLT(val)  (((val) * 10000) + 600000)
#define MT6682A_I2C_SLAVE	       0x64

enum {
	REGULATOR_BUCK1,
	REGULATOR_BUCK2,
	REGULATOR_BUCK3,
	REGULATOR_BUCK4,
	REGULATOR_LDO,
	REGULATOR_MAX_NR,
};

struct regulator_info {
    unsigned int min_uV;
    unsigned int max_uV;
    unsigned short vol_reg;
    unsigned short vol_mask;
    unsigned short vol_shift;
    unsigned short enable_reg;
    unsigned short enable_shift;
    unsigned short mode_reg;
    unsigned short mode_shift;
    unsigned short step_uV;
};

/* MT6682A Registers */
#define MT6682A_REG_BUCK1             (0x03)
#define MT6682A_REG_BUCK2_SEL         (0x04)
#define MT6682A_REG_BUCK3_SEL         (0x05)
#define MT6682A_REG_DCDCCTRL0         (0x06)
#define MT6682A_REG_ENABLE            (0x07)
#define MT6682A_REG_DCDCCTRL1         (0x08)
#define MT6682A_REG_DISCHARGE         (0x09)
#define MT6682A_REG_UV_PROTECTION     (0x0A)
#define MT6682A_REG_MUTE_CONTROL      (0x0B)
#define MT6682A_REG_PGOOD_STATUS      (0x0F)
#define MT6682A_REG_OV_INT            (0x10)
#define MT6682A_REG_UV_INT            (0x11)
#define MT6682A_REG_PG_INT            (0x12)
#define MT6682A_REG_HOT_DIE_INT       (0x13)
#define MT6682A_REG_OV_MASK           (0x14)
#define MT6682A_REG_UV_MASK           (0x15)
#define MT6682A_REG_PG_MASK           (0x16)
#define MT6682A_REG_HOT_DIE_MASK      (0x17)
#define MT6682A_REG_VIN1_OK_CTRL_RAIL (0x2A)


/* buck1 info */
#define buck1_vol_reg          0
#define buck1_vol_mask         0
#define buck1_vol_shift        0
#define buck1_enable_reg       MT6682A_REG_ENABLE
#define buck1_enable_shift     1
#define buck1_mode_reg         MT6682A_REG_DCDCCTRL0
#define buck1_mode_shift       1
#define buck1_min_uV           0
#define buck1_max_uV           0
#define buck1_step_uV          0

/* buck2 info */
#define buck2_vol_reg          MT6682A_REG_BUCK2_SEL
#define buck2_vol_mask         0x7F
#define buck2_vol_shift        0
#define buck2_enable_reg       MT6682A_REG_ENABLE
#define buck2_enable_shift     2
#define buck2_mode_reg         0
#define buck2_mode_shift       0
#define buck2_min_uV           600000
#define buck2_max_uV           1400000
#define buck2_step_uV          10000

/* buck3 info */
#define buck3_vol_reg          MT6682A_REG_BUCK3_SEL
#define buck3_vol_mask         0x7F
#define buck3_vol_shift        0
#define buck3_enable_reg       MT6682A_REG_ENABLE
#define buck3_enable_shift     3
#define buck3_mode_reg         0
#define buck3_mode_shift       0
#define buck3_min_uV           600000
#define buck3_max_uV           1400000
#define buck3_step_uV          10000

/* buck4 info */
#define buck4_vol_reg          0
#define buck4_vol_mask         0
#define buck4_vol_shift        0
#define buck4_enable_reg       MT6682A_REG_ENABLE
#define buck4_enable_shift     4
#define buck4_mode_reg         MT6682A_REG_DCDCCTRL0
#define buck4_mode_shift       4
#define buck4_min_uV           0
#define buck4_max_uV           0
#define buck4_step_uV          0

/* ldo info */
#define ldo_vol_reg          0
#define ldo_vol_mask         0
#define ldo_vol_shift        0
#define ldo_enable_reg       MT6682A_REG_ENABLE
#define ldo_enable_shift     5
#define ldo_mode_reg         0
#define ldo_mode_shift       0
#define ldo_min_uV           0
#define ldo_max_uV           0
#define ldo_step_uV          0

#define mt6682a_decl(_name)\
{ \
    .min_uV = _name##_min_uV, \
    .max_uV = _name##_max_uV, \
    .vol_reg = _name##_vol_reg, \
    .vol_mask = _name##_vol_mask, \
    .vol_shift = _name##_vol_shift, \
    .enable_reg = _name##_enable_reg, \
    .enable_shift = _name##_enable_shift, \
    .mode_reg = _name##_mode_reg, \
    .mode_shift = _name##_mode_shift, \
    .step_uV = _name##_step_uV, \
}

//---------------------- EXPORT API ---------------------------
int mt6682a_set_voltage(uint8_t id, int volt, int max_volt);
int mt6682a_get_voltage(uint8_t id);
int mt6682a_enable(uint8_t id, uint8_t en);
int mt6682a_set_mode(uint8_t id, uint8_t mode);
void mt6682a_init(void);

#endif /* MT6682A_H */
