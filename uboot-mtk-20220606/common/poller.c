// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2010 Marc Kleine-Budde <mkl@pengutronix.de>
 */

#include <common.h>
#include <malloc.h>
#include <poller.h>

static LIST_HEAD(poller_list);
static int __poller_active;

bool poller_active(void)
{
	return __poller_active;
}

int poller_register(struct poller_struct *poller, const char *name)
{
	if (poller->registered)
		return -EBUSY;

	poller->name = strdup(name);
	list_add_tail(&poller->list, &poller_list);
	poller->registered = 1;

	return 0;
}

int poller_unregister(struct poller_struct *poller)
{
	if (!poller->registered)
		return -ENODEV;


	list_del(&poller->list);
	poller->registered = 0;
	free(poller->name);

	return 0;
}

static void poller_async_callback(struct poller_struct *poller)
{
	struct poller_async *pa = container_of(poller, struct poller_async, poller);

	if (!pa->active)
		return;

	if (timer_get_us() < pa->end)
		return;

	pa->active = 0;
	pa->fn(pa->ctx);
}

/*
 * Cancel an outstanding asynchronous function call
 *
 * @pa		the poller that has been scheduled
 *
 * Cancel an outstanding function call. Returns 0 if the call
 * has actually been cancelled or -ENODEV when the call wasn't
 * scheduled.
 *
 */
int poller_async_cancel(struct poller_async *pa)
{
	pa->active = 0;

	return 0;
}

/*
 * Call a function asynchronously
 *
 * @pa		the poller to be used
 * @delay	The delay in nanoseconds
 * @fn		The function to call
 * @ctx		context pointer passed to the function
 *
 * This calls the passed function after a delay of delay_us. Returns
 * a pointer which can be used as a cookie to cancel a scheduled call.
 */
int poller_call_async(struct poller_async *pa, uint64_t delay_us,
		void (*fn)(void *), void *ctx)
{
	pa->ctx = ctx;
	pa->end = timer_get_us() + delay_us;
	pa->fn = fn;
	pa->active = 1;

	return 0;
}

int poller_async_register(struct poller_async *pa, const char *name)
{
	pa->poller.func = poller_async_callback;
	pa->active = 0;

	return poller_register(&pa->poller, name);
}

int poller_async_unregister(struct poller_async *pa)
{
	return poller_unregister(&pa->poller);
}

void poller_call(void)
{
	struct poller_struct *poller, *tmp;

	__poller_active = 1;

	list_for_each_entry_safe(poller, tmp, &poller_list, list)
		poller->func(poller);

	__poller_active = 0;
}
