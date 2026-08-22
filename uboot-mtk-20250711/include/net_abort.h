/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright (C) 2026 Yuzhii0718
 *
 * Network boot abort support (uBootEnter.py / BreedEnter compatible)
 *
 * Listens for the "UBOOT:ABORT" / "BREED:ABORT" magic packets (UDP port
 * 37541) while the boot countdown is running.  Once detected, the web
 * failsafe (httpd) is entered so that the host tool can recover the device
 * over HTTP without console access.  The matching "<PROTO>:ABORTED" reply
 * is broadcast on port 37540.
 */

#ifndef __NET_ABORT_H
#define __NET_ABORT_H

#include <linux/types.h>

/**
 * net_abort_prepare() - initialise network abort detection
 *
 * Called just before the bootdelay/menu countdown starts.  Initialises the
 * ethernet device and installs the UDP listener.  This is a no-op when the
 * environment variable "disable_net_abort" is set.
 */
void net_abort_prepare(void);

/**
 * net_abort_poll() - pump received packets during countdown loops
 *
 * Must be called periodically (e.g. every 10ms) from the boot countdown
 * loops so that incoming UDP packets are processed.
 */
void net_abort_poll(void);

/**
 * net_abort_detected() - has the abort magic packet been received?
 *
 * Return: true when the "UBOOT:ABORT" packet has been received.
 */
bool net_abort_detected(void);

/**
 * net_abort_finish() - tear down the abort listener
 *
 * Restores the previously installed UDP handler.  When an abort was
 * detected, broadcasts the "UBOOT:ABORTED" reply and enters the blocking
 * web failsafe (httpd).
 */
void net_abort_finish(void);

#endif /* __NET_ABORT_H */
