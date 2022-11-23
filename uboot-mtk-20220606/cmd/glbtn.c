#include <common.h>
#include <command.h>
#include <button.h>
#include <linux/delay.h>
#include <poller.h>

static struct poller_async led_p;

void led_control(const char *cmd, const char *name, const char *arg)
{
	const char *led = env_get(name);
	char buf[128];

	if (!led)
		return;

	sprintf(buf, "%s %s %s", cmd, led, arg);

	run_command(buf, 0);
}

static void usb_power_clr(void)
{
	const char *num = env_get("gpio_usb_power");
	char buf[128];

	if (!num)
		return;

	sprintf(buf, "gpio clear %s", num);
	run_command(buf, 0);
}

static void led_action_post(void *arg)
{
	led_control("ledblink", "blink_led", "0");
	led_control("led", "blink_led", "on");
}

static int do_glbtn(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	const char *button_label = "reset";
	int ret, counter = 0;
	struct udevice *dev;
	ulong ts;

	led_control("ledblink", "blink_led", "250");

	usb_power_clr();

	ret = button_get_by_label(button_label, &dev);
	if (ret) {
		printf("Button '%s' not found (err=%d)\n", button_label, ret);
		return CMD_RET_FAILURE;
	}

	if (!button_get_state(dev)) {
		poller_async_register(&led_p, "led_pa");
		poller_call_async(&led_p, 1000000, led_action_post, NULL);
		return CMD_RET_SUCCESS;
	}

	led_control("ledblink", "blink_led", "500");

	printf("RESET button is pressed for: %2d second(s)", counter++);

	ts = get_timer(0);

	while (button_get_state(dev) && counter < 6) {
		if (get_timer(ts) < 1000)
			continue;

		ts = get_timer(0);

		printf("\b\b\b\b\b\b\b\b\b\b\b\b%2d second(s)", counter++);
	}

	printf("\n");

	led_control("ledblink", "blink_led", "0");

	if (counter == 6) {
		led_control("led", "system_led", "on");
		run_command("httpd", 0);
	} else {
		led_control("ledblink", "blink_led", "0");
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	glbtn, 1, 0, do_glbtn,
	"GL-iNet button check",
	""
);
