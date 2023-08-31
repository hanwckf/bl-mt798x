// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <led.h>
#include <poller.h>
#include <time.h>

#define MAX_LED_BLINK 5

struct led_blink {
	struct udevice *dev;
	int freq_ms;
	uint64_t next_event;
};

static struct led_blink led_blinks[MAX_LED_BLINK];

static void led_blink_func(struct poller_struct *poller)
{
	uint64_t now = timer_get_us() / 1000;
	int i;

	for (i = 0; i < MAX_LED_BLINK; i++) {
		struct led_blink *led = &led_blinks[i];
		if (!led->dev)
			continue;

		if (led->next_event && now < led->next_event)
			continue;

		led_set_state(led->dev, LEDST_TOGGLE);

		led->next_event = now + led->freq_ms;
	}
}

static struct poller_struct led_poller = {
	.func = led_blink_func,
};

static int do_led_blink(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	struct led_blink *led = NULL;
	struct udevice *dev;
	const char *label;
	int freq_ms = 0;
	int i, ret;

	if (argc < 3)
		return CMD_RET_USAGE;

	label = argv[1];
	freq_ms = dectoul(argv[2], NULL);

	ret = led_get_by_label(label, &dev);
	if (ret) {
		printf("LED '%s' not found (err=%d)\n", label, ret);
		return CMD_RET_FAILURE;
	}

	for (i = 0; i < MAX_LED_BLINK; i++) {
		if (led_blinks[i].dev == dev) {
			led = &led_blinks[i];
			break;
		}
	}

	if (freq_ms < 1) {
		led->dev = NULL;
		led_set_state(dev, LEDST_OFF);
		return 0;
	}

	if (!led) {
		for (i = 0; i < MAX_LED_BLINK; i++) {
			if (!led_blinks[i].dev) {
				led = &led_blinks[i];
				led->dev = dev;
				break;
			}
		}
	}

	if (!led)
		return CMD_RET_FAILURE;

	led_set_state(dev, LEDST_TOGGLE);

	led->freq_ms = freq_ms;

	poller_register(&led_poller, "led");

	return 0;
}

U_BOOT_CMD(
	ledblink, 3, 0, do_led_blink,
	"led blink",
	"<led_label> [blink-freq in ms]"
);
