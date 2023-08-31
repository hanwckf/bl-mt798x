// SPDX-License-Identifier: GPL-2.0+
/*
 * Color LED demo
 *
 * Copyright (c) 2022 MediaTek, Inc
 * author: Sam Shih <sam.shih@mediatek.com>
 */

#include <asm/gpio.h>
#include <command.h>
#include <cyclic.h>
#include <dm.h>
#include <linux/delay.h>
#include <dm/pinctrl.h>
#include <pwm.h>

#define PWM_DEFAULT_FREQ	100000	/* 100Khz */
#define CLED_DEMO_DELAY		1	/* 1ms */

#define MAX_CLED_FRAME_BUF	3000
#define MAX_CLED_NUM		10

#define CLED_ENABLE		BIT(0)
#define CLED_RESTART		BIT(1)
#define CLED_LOOP		BIT(2)
#define CLED_FRAME_BUFFER	BIT(3)

#define CLED_PWM		BIT(4)
#define CLED_GPIO		BIT(5)

/* CLED pwm demo color table
 *
 *    Turn1 Trun2 Trun3 Turn4 Trun5 Trun6
 * R: 255,  255,  000,  000,  000,  255
 * G: 000,  000,  000,  255,  255,  255
 * B: 000,  255,  255,  255,  000,  000
 */

const unsigned char rT[] = {255, 255, 0, 0, 0, 255};
const unsigned char gT[] = {0, 0, 0, 255, 255, 255};
const unsigned char bT[] = {0, 255, 255, 255, 0, 0};

struct cled_info {
	const char *name;
	struct cyclic_info *cyclic;
	u32 frame_buffer[MAX_CLED_FRAME_BUF];
	u32 frame_buffer_len;
	u32 frame_buffer_index;
	/* pwm */
	struct udevice *pwm_dev;
	u32 pwm_ch[3];
	u32 pwm_period;
	/* gpio */
	u32 gpio[3];
	u32 flags;
};

struct cled_info cled_list[MAX_CLED_NUM];
int cled_index = 0;

static int update_cled_gpio(struct cled_info *info, u8 r, u8 g, u8 b) {
	if (r < 128)
		gpio_direction_output(info->gpio[0], 0);
	else
		gpio_direction_output(info->gpio[0], 1);
	if (g < 128)
		gpio_direction_output(info->gpio[1], 0);
	else
		gpio_direction_output(info->gpio[1], 1);
	if (b < 128)
		gpio_direction_output(info->gpio[2], 0);
	else
		gpio_direction_output(info->gpio[2], 1);

	return 0;
}

static int update_cled_pwm(struct cled_info *info, u8 r, u8 g, u8 b) {
	pwm_set_config(info->pwm_dev, info->pwm_ch[0], info->pwm_period,
		       (u32)(r * (info->pwm_period / 255)));
	pwm_set_config(info->pwm_dev, info->pwm_ch[1], info->pwm_period,
		       (u32)(g * (info->pwm_period / 255)));
	pwm_set_config(info->pwm_dev, info->pwm_ch[2], info->pwm_period,
		       (u32)(b * (info->pwm_period / 255)));

	return 0;
}

static int update_cled(struct cled_info *info) {
	u8 r, g, b;

	if (info->flags & CLED_RESTART) {
		info->frame_buffer_index = 0;
		info->flags &= ~CLED_RESTART;
	}

	if (info->frame_buffer_index >= info->frame_buffer_len) {
		if (info->flags & CLED_LOOP)
			info->frame_buffer_index = 0;
		else
			return 0;
	}

	r = (info->frame_buffer[info->frame_buffer_index] & 0x00ff0000) >> 16;
	g = (info->frame_buffer[info->frame_buffer_index] & 0x0000ff00) >> 8;
	b = (info->frame_buffer[info->frame_buffer_index] & 0x000000ff) >> 0;

	if(info->flags & CLED_GPIO)
		update_cled_gpio(info, r, g, b);
	if(info->flags & CLED_PWM)
		update_cled_pwm(info, r, g, b);

	info->frame_buffer_index++;

	return 0;
}

static void cled_cyclic(void *ctx)
{
	struct cled_info *info = ctx;

	if (info->flags & CLED_ENABLE)
		update_cled(info);
}

static struct cled_info * init_cled_pwm(const char *name, u32 pwm_dev_id,
					u32 pwm_freq, u32 pwm_ch_r,
					u32 pwm_ch_g, u32 pwm_ch_b)
{
	struct cled_info *info;
	int i, ret;

	for (i=0 ; i<MAX_CLED_NUM ; i++) {
		info = &cled_list[i];
		if (!strcmp(info->name, name))
			return info;
	}

	info = &cled_list[cled_index++];

	ret = uclass_get_device(UCLASS_PWM, pwm_dev_id, &info->pwm_dev);
	if (ret) {
		printf("Can't get pwm device-%d (%d)\n", pwm_dev_id, ret);
		return NULL;
	}

	info->name = name;
	info->pwm_ch[0] = pwm_ch_r;
	info->pwm_ch[1] = pwm_ch_g;
	info->pwm_ch[2] = pwm_ch_b;
	info->pwm_period = 1000000000 / pwm_freq;
	info->flags = 0;

	for (i = 0 ; i < 3 ; i++) {
		ret = pwm_set_config(info->pwm_dev, info->pwm_ch[i],
				     info->pwm_period, 0);
		if (ret) {
			printf("error(%d)\n", ret);
			return NULL;
		}
		ret = pwm_set_enable(info->pwm_dev, info->pwm_ch[i], 1);
		if (ret) {
			printf("error(%d)\n", ret);
			return NULL;
		}
	}

	info->cyclic = cyclic_register(cled_cyclic, CLED_DEMO_DELAY * 1000,
				       info->name, info);
	if (!info->cyclic) {
		printf("Registering of cled_cyclic failed\n");
		return NULL;
	}

	info->flags |= CLED_PWM;

	return info;
}

static struct cled_info * init_cled_gpio(const char *name, u32 gpio_r,
					 u32 gpio_g, u32 gpio_b)
{
	struct cled_info *info;
	u32 gpio[3];
	int i, ret;

	for (i=0 ; i<MAX_CLED_NUM ; i++) {
		info = &cled_list[i];
		if (!strcmp(info->name, name))
			return info;
	}

	info = &cled_list[cled_index++];

	gpio[0] = gpio_r;
	gpio[1] = gpio_g;
	gpio[2] = gpio_b;

	for (i = 0 ; i < 3 ; i++) {
		ret = gpio_request(gpio[i], "cled");
		if (ret && ret != -EBUSY) {
			printf("gpio: requesting pin %u failed\n", gpio[i]);
			return NULL;
		}
	}

	info->name = name;
	info->gpio[0] = gpio[0];
	info->gpio[1] = gpio[1];
	info->gpio[2] = gpio[2];
	info->flags = 0;
	info->cyclic = cyclic_register(cled_cyclic, CLED_DEMO_DELAY * 1000,
				       info->name, info);
	if (!info->cyclic) {
		printf("Registering of cled_cyclic failed\n");
		return NULL;
	}

	gpio_direction_output(info->gpio[0], 0);
	gpio_direction_output(info->gpio[1], 0);
	gpio_direction_output(info->gpio[2], 0);

	info->flags |= CLED_GPIO;

	return info;
}

static void remove_cled(const char *name) {
	struct cled_info *info;
	int i;

	for (i=0 ; i<MAX_CLED_NUM ; i++) {
		info = &cled_list[i];
		if (!strcmp(info->name, name)) {
			cyclic_unregister(info->cyclic);
			info->name = NULL;
		}
	}
}

static int init_cled_frame_buffer(struct cled_info *info,
				  const unsigned char *rT,
				  const unsigned char *gT,
				  const unsigned char *bT,
				  const unsigned char numT,
				  const unsigned char color_step)
{
	unsigned char r, g, b;
	char rS, gS, bS;
	int i, j;
	u32 v;

	if (info->flags & CLED_FRAME_BUFFER)
		return 0;

	info->frame_buffer_len = 0;
	info->frame_buffer_index = 0;

	for (i = 0 ; i < numT ; i++) {
		r = rT[i];
		g = gT[i];
		b = bT[i];
		if ((i + 1) == numT) {
			rS = (rT[0] - rT[i]) / color_step;
			gS = (gT[0] - gT[i]) / color_step;
			bS = (bT[0] - bT[i]) / color_step;
		} else {
			rS = (rT[i + 1] - rT[i]) / color_step;
			gS = (gT[i + 1] - gT[i]) / color_step;
			bS = (bT[i + 1] - bT[i]) / color_step;
		}
		for (j = 0 ; j < color_step ; j+=1) {
			r += rS;
			g += gS;
			b += bS;
			if ((info->frame_buffer_len + 1) < MAX_CLED_FRAME_BUF) {
				v = ((r << 16) | (g << 8) | (b));
				info->frame_buffer[info->frame_buffer_len] = v;
				info->frame_buffer_len++;
			} else {
				return -ENOBUFS;
			}
		}
	}

	info->flags |= CLED_FRAME_BUFFER;

	return 0;
}

struct mtk_led_demo {
	const char *demo_name;
	const char *desc;
	unsigned int pins[3];
	unsigned int pin_gpio_mode[3];
	unsigned int pin_pwm_mode[3];
	unsigned int pwm_dev;
	unsigned int pwm_ch[3];
	const char *pinctrl_dev_name;
};

static const struct mtk_led_demo mtk_led_demo_data[] = {
	{
		.demo_name = "mt7981",
		.desc = "MT7981_LED_DEMO_1",
		.pins = {15, 14, 7},
		.pin_gpio_mode = {0, 0, 0},
		.pin_pwm_mode = {1, 2, 4},
		.pwm_dev = 0,
		.pwm_ch = {0, 1, 2},
		.pinctrl_dev_name = "pinctrl@11d00000",
	},
	{
		.demo_name = "mt7988_1",
		.desc = "MT7988_LED_DEMO_1",
		.pins = {57, 21, 80},
		.pin_gpio_mode = {0, 0, 0},
		.pin_pwm_mode = {1, 1, 2},
		.pwm_dev = 0,
		.pwm_ch = {0, 1, 2},
		.pinctrl_dev_name = "pinctrl@1001f000",
	},
	{
		.demo_name = "mt7988_2",
		.desc = "MT7988_LED_DEMO_2",
		.pins = {81, 82, 83},
		.pin_gpio_mode = {0, 0, 0},
		.pin_pwm_mode = {2, 2, 2},
		.pwm_dev = 0,
		.pwm_ch = {3, 4, 5},
		.pinctrl_dev_name = "pinctrl@1001f000",
	},
	{}
};

static int mtk_led_demo_init_pinctrl(const struct mtk_led_demo *info,
				     const char *mode)
{
	const struct pinctrl_ops *ops;
	struct udevice *pinctrl_dev;
	int func, i, ret;

	ret = uclass_get_device_by_name(UCLASS_PINCTRL, info->pinctrl_dev_name,
					&pinctrl_dev);
	if (ret) {
		printf("Can't get the pin-controller: %s!\n",
		       info->pinctrl_dev_name);
		return -1;
	}

	ops = pinctrl_get_ops(pinctrl_dev);

	if (ops->pinmux_set) {
		for (i=0 ; i<3 ; i++) {
			if (!strcmp(mode, "pwm"))
				func = info->pin_pwm_mode[i];
			else if (!strcmp(mode, "gpio"))
				func = info->pin_gpio_mode[i];
			else
				return -1;
			ret = ops->pinmux_set(pinctrl_dev, info->pins[i], func);
			if (ret) {
				printf("Can't set pin%d to func%d\n",
				       info->pins[i], func);
				return -1;
			}
		}

	}

	return 0;
}

static int do_cled_demo(struct cmd_tbl *cmdtp, int flag, int argc,
			char *const argv[])
{
	const struct mtk_led_demo *data = NULL;
	struct cled_info *cled;
	const char *demo_name;
	const char *type;
	int i, ret;

	demo_name = *argv;
	argc--;
	argv++;

	if (!demo_name)
		return CMD_RET_USAGE;

	for (i=0 ; i<ARRAY_SIZE(mtk_led_demo_data) ; i++)
		if (!strcmp((&mtk_led_demo_data[i])->demo_name, demo_name))
			data = &mtk_led_demo_data[i];

	if (!data) {
		printf("ERROR: Demo '%s' not found in cled demo data\n", demo_name);
		return CMD_RET_USAGE;
	}

	type = *argv;
	argc--;
	argv++;

	if (strcmp(type, "pwm") && strcmp(type, "gpio")) {
		printf("ERROR: demo type '%s' not supported\n", type);
		return CMD_RET_FAILURE;
	}

	remove_cled(data->desc);
	ret = mtk_led_demo_init_pinctrl(data, type);
	if (ret)
		printf("WARN: unable to configure pinmux by pinctrl\n");

	if (!strcmp(type, "pwm")) {
		/* Cled pwm demo */
		cled = init_cled_pwm(data->desc, data->pwm_dev,
				     PWM_DEFAULT_FREQ, data->pwm_ch[0],
				     data->pwm_ch[1], data->pwm_ch[2]);
	}

	if (!strcmp(type, "gpio")) {
		/* Cled gpio demo */
		cled = init_cled_gpio(data->desc, data->pins[0], data->pins[1],
				      data->pins[2]);
	}

	if (!cled)
		return CMD_RET_FAILURE;

	/* Setup frame buffer */
	init_cled_frame_buffer(cled, rT, gT, bT, 6, 255);
	/* Setup flags to run cled */
	cled->flags |= (CLED_RESTART | CLED_LOOP | CLED_ENABLE);

	return CMD_RET_SUCCESS;
};

static int do_cled(struct cmd_tbl *cmdtp, int flag, int argc,
		   char *const argv[])
{
	const char *str_cmd;

	if (argc < 4)
		return CMD_RET_USAGE;

	str_cmd = argv[1];
	argc -= 2;
	argv += 2;

	switch (*str_cmd) {
	case 'd':
		if (argc < 2)
			return CMD_RET_USAGE;
		return do_cled_demo(cmdtp, flag, argc, argv);
	default:
		return CMD_RET_USAGE;
	}
}

U_BOOT_CMD(cled, 6, 0, do_cled,
	   "color led demo",
	   "demo <demo_name> <type> - start cled demo\n"
	   "valid demo_name: [mt7981, mt7988_1, mt7988_2]\n"
	   "valid type: [gpio, pwm]\n"
	   "\nexample:\ncled demo mt7988 pwm\n"
);
