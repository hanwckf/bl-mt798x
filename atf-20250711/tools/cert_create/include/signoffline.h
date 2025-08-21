/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-CLause
 */

#ifndef SIGNOFFLINE_H
#define SIGNOFFLINE_H

#include <errno.h>
#include <stdbool.h>

struct offsign_s {
	int signoffline;
	cert_key_t dummy_key;
};

extern struct offsign_s offsign;

int key_load_pub(cert_key_t *key);

int offline_sign(X509 **x, cert_t *cert, cert_key_t *ikey, int md_alg);
#endif /* SIGNOFFLINE_H */
