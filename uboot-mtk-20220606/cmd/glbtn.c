#include <common.h>
#include <command.h>
#include <button.h>
#include <linux/delay.h>
#include <poller.h>

static struct poller_async led_p;

static void led_action_post(void *arg)
{
	run_command("ledblink blue:run 0", 0);
	run_command("led blue:run on", 0);
}

static int do_glbtn(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	const char *button_label = "reset";
	int ret, counter = 0;
	struct udevice *dev;
	ulong ts;

	run_command("ledblink blue:run 250", 0);
	//run_command("gpio clear 12", 0);
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

	run_command("ledblink blue:run 500", 0);

	printf("RESET button is pressed for: %2d second(s)", counter++);

	ts = get_timer(0);

	while (button_get_state(dev) && counter < 6) {
		if (get_timer(ts) < 1000)
			continue;

		ts = get_timer(0);

		printf("\b\b\b\b\b\b\b\b\b\b\b\b%2d second(s)", counter++);
	}

	printf("\n");

	run_command("ledblink blue:run 0", 0);

	if (counter == 6) {
		run_command("led white:system on", 0);
		run_command("httpd", 0);
	} else {
		run_command("led blue:run on", 0);
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
	glbtn, 1, 0, do_glbtn,
	"GL-iNet button check",
	""
);
