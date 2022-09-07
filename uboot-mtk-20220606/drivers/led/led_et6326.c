// SPDX-License-Identifier: GPL-2.0+

#include <common.h>
#include <dm.h>
#include <errno.h>
#include <led.h>
#include <log.h>
#include <malloc.h>
#include <dm/lists.h>
#include <i2c.h>

#define ET6326_MAX_LED_NUM			3

#define ET6326_REG_STATUS			0x00
#define ET6326_REG_FLASH_PERIOD		0x01
#define ET6326_REG_FLASH_TON1		0x02
#define ET6326_REG_FLASH_TON2		0x03
#define ET6326_REG_LED_WORK_MODE	0x04
#define ET6326_REG_RAMP_RATE		0x05
#define ET6326_REG_LED1_IOUT		0x06
#define ET6326_REG_LED2_IOUT		0x07
#define ET6326_REG_LED3_IOUT		0x08

#define ET6326_MAX_PERIOD			16380 	/* ms */
#define ET6326_PATTERN_CNT			2

enum {
	MODE_ALWAYS_OFF,
	MODE_ALWAYS_ON,
	MODE_THREAD
};

struct et6326_priv {
	struct udevice *i2c;
};

struct et6326_led_priv {
	struct et6326_priv *priv;
	int period;
	int mode;
	u8 mode_mask;
	u8 on_shift;
	u8 blink_shift;
	u8 reg_iout;
};

static int et6326_set_work_mode(struct et6326_led_priv *led, int mode)
{
	struct et6326_priv *priv = led->priv;
	int old_val, new_val;

	old_val = dm_i2c_reg_read(priv->i2c, ET6326_REG_LED_WORK_MODE);
	if (old_val < 0)
		return old_val;

	new_val = old_val & ~led->mode_mask;

	if (mode == MODE_ALWAYS_ON)
		new_val |= (1 << led->on_shift);
	else if (mode == MODE_THREAD)
		new_val |= 1 << led->blink_shift;

	if (new_val == old_val)
		return 0;

	led->mode = mode;

	return dm_i2c_reg_write(priv->i2c, ET6326_REG_LED_WORK_MODE, new_val);
}

static void et6326_set_ton1_percentage(struct et6326_priv *priv, u8 val)
{
	if (val > 100)
		val = 100;

	val = 255 * val / 100;

	dm_i2c_reg_write(priv->i2c, ET6326_REG_FLASH_TON1, val);
}

static void et6326_set_ramp(struct et6326_priv *priv, u32 ms)
{
	int scaling_reg;
	u8 ramp_reg;

	if (ms > 7680)
		ms = 7680;

	if (ms < 2048) {
		scaling_reg = 0;
		ramp_reg = ms / 128;
	} else if (ms < 4096) {
		scaling_reg = 1;
		ramp_reg = ms / 256;
	} else {
		scaling_reg = 2;
		ramp_reg = ms / 512;
	}

	ramp_reg |= ramp_reg << 4;

	dm_i2c_reg_write(priv->i2c, ET6326_REG_STATUS, scaling_reg << 5);
	dm_i2c_reg_write(priv->i2c, ET6326_REG_RAMP_RATE, ramp_reg);
}

static int et6326_led_set_state(struct udevice *dev, enum led_state_t state)
{
	struct et6326_led_priv *led = dev_get_priv(dev);
	struct et6326_priv *priv = led->priv;

	switch (state) {
	case LEDST_OFF:
		et6326_set_work_mode(led, MODE_ALWAYS_OFF);
		break;
	case LEDST_ON:
		et6326_set_work_mode(led, MODE_ALWAYS_ON);
		dm_i2c_reg_write(priv->i2c, led->reg_iout, 255);
		break;
	case LEDST_TOGGLE:
		et6326_set_work_mode(led, !led->mode);
		break;
#ifdef CONFIG_LED_BLINK
	case LEDST_BLINK:
		dm_i2c_reg_write(priv->i2c, ET6326_REG_FLASH_PERIOD, led->period / 128 - 1);
		et6326_set_ton1_percentage(priv, led->period / 2 * 100 / led->period);
		et6326_set_ramp(priv, 0);
		et6326_set_work_mode(led, MODE_THREAD);
		break;
#endif
	default:
		return 0;
	}

	if (led->mode != MODE_ALWAYS_OFF)
		dm_i2c_reg_write(priv->i2c, led->reg_iout, 255);

	return 0;
}

static enum led_state_t et6326_led_get_state(struct udevice *dev)
{
	struct et6326_led_priv *led = dev_get_priv(dev);
	int v = dm_i2c_reg_read(led->priv->i2c, led->reg_iout);

	if (v > 0)
		return LEDST_ON;

	return LEDST_OFF;
}

#ifdef CONFIG_LED_BLINK
static int et6326_led_set_period(struct udevice *dev, int period)
{
	struct et6326_led_priv *led = dev_get_priv(dev);
	led->period = period;
	return 0;
}
#endif

static int led_et6326_probe(struct udevice *dev)
{
	struct et6326_led_priv *led = dev_get_priv(dev);
	u32 channel;

	dev_read_u32(dev, "channel", &channel);

	switch (channel) {
	case 0:
		led->mode_mask = 0x03;
		led->on_shift = 0;
		led->blink_shift = 1;
		break;
	case 1:
		led->mode_mask = (0x03 << 2) | (1 << 6);
		led->on_shift = 2;
		led->blink_shift = 3;
		break;
	case 2:
		led->mode_mask = (0x03 << 4) | (1 << 7);
		led->on_shift = 4;
		led->blink_shift = 5;
		break;
	default:
		return -EINVAL;
	}

	led->reg_iout = ET6326_REG_LED1_IOUT + channel;

	return 0;
}

static int led_et6326_remove(struct udevice *dev)
{
	return 0;
}

static int led_et6326_bind(struct udevice *parent)
{
	struct et6326_priv *priv = dev_get_priv(parent);
	struct et6326_led_priv *led;
	struct udevice *dev;
	ofnode node;
	u32 addr;
	int ret;

	ret = dev_read_u32(parent, "addr", &addr);
	if (ret)
		return ret;

	ret = i2c_get_chip_for_busnum(0, addr, 1, &priv->i2c);
	if (ret) {
		debug("%s: Cannot find ET6326 I2C chip\n", __func__);
		return ret;
	}

	dev_for_each_subnode(node, parent) {
		ret = device_bind_driver_to_node(parent, "et6326_led",
						 ofnode_get_name(node),
						 node, &dev);
		if (ret)
			return ret;

		led = dev_get_priv(dev);
		led->priv = priv;
	}

	return 0;
}

static const struct led_ops et6326_led_ops = {
	.set_state	= et6326_led_set_state,
	.get_state	= et6326_led_get_state,
#ifdef CONFIG_LED_BLINK
	.set_period = et6326_led_set_period
#endif
};

U_BOOT_DRIVER(led_et6326) = {
	.name = "et6326_led",
	.id	= UCLASS_LED,
	.ops = &et6326_led_ops,
	.priv_auto	= sizeof(struct et6326_led_priv),
	.probe	= led_et6326_probe,
	.remove	= led_et6326_remove,
};

static const struct udevice_id led_et6326_ids[] = {
	{ .compatible = "etek,et6326" },
	{ }
};

U_BOOT_DRIVER(led_et6326_wrap) = {
	.name	= "et6326_led_wrap",
	.id	= UCLASS_NOP,
	.of_match = led_et6326_ids,
	.priv_auto	= sizeof(struct et6326_priv),
	.bind	= led_et6326_bind,
};
