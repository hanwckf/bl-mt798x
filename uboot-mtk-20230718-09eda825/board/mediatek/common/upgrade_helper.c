// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data upgrading helper
 */

#include <errno.h>
#include <linux/bitops.h>
#include <linux/sizes.h>
#include <linux/types.h>
#include <u-boot/crc.h>

#include "load_data.h"
#include "upgrade_helper.h"
#include "colored_print.h"

int check_data_size(u64 total_size, u64 offset, size_t max_size, size_t size,
		    bool write)
{
	if (total_size < offset || size > total_size - offset) {
		printf("\n");
		cprintln(ERROR, "*** Data %s pasts flash size! ***",
			 write ? "write" : "read");
		goto abort;
	}

	if (!max_size || size <= max_size)
		return 0;

	printf("\n");
	cprintln(ERROR, "*** Data is too large (0x%zx > 0x%zx)! ***", size,
		 max_size);

abort:
	cprintln(ERROR, "*** Operation Aborted! ***");

	return -EINVAL;
}

bool verify_data(const u8 *orig, const u8 *rdback, u64 offset, size_t size)
{
	bool passed = true;
	size_t i;

	for (i = 0; i < size; i++) {
		if (orig[i] != rdback[i]) {
			passed = false;
			break;
		}
	}

	if (passed)
		return true;

	printf("Fail\n");
	cprintln(ERROR, "*** Verification failed at 0x%llx! ***",
		 offset + i);
	cprintln(ERROR, "*** Expected 0x%02x, got 0x%02x ***", orig[i],
		 rdback[i]);
	cprintln(ERROR, "*** Data is damaged, please retry! ***");

	return false;
}
