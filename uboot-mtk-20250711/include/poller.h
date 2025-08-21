/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2010 Marc Kleine-Budde <mkl@pengutronix.de>
 */

#ifndef POLLER_H
#define POLLER_H

#include <linux/list.h>
#include <linux/types.h>

struct poller_struct {
	void (*func)(struct poller_struct *poller);
	int registered;
	struct list_head list;
	char *name;
};

int poller_register(struct poller_struct *poller, const char *name);
int poller_unregister(struct poller_struct *poller);

struct poller_async;

struct poller_async {
	struct poller_struct poller;
	void (*fn)(void *);
	void *ctx;
	uint64_t end;
	int active;
};

int poller_async_register(struct poller_async *pa, const char *name);
int poller_async_unregister(struct poller_async *pa);

int poller_call_async(struct poller_async *pa, uint64_t delay_us,
		void (*fn)(void *), void *ctx);
int poller_async_cancel(struct poller_async *pa);
static inline bool poller_async_active(struct poller_async *pa)
{
	return pa->active;
}

#ifdef CONFIG_POLLER
bool poller_active(void);
void poller_call(void);
#else
static inline bool poller_active(void)
{
	return false;
}
static inline void poller_call(void)
{
}
#endif	/* CONFIG_POLLER */

#endif	/* !POLLER_H */
