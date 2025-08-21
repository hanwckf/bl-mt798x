// SPDX-License-Identifier: GPL-2.0+

#include <command.h>
#include <env.h>
#include <errno.h>
#include <mapmem.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <vsprintf.h>
#include "unxz.h"

static int do_xzdec(struct cmd_tbl *cmdtp, int flag, int argc,
		      char *const argv[])
{
	uintptr_t out_buf_addr;
	uintptr_t in_buf_addr;
	const void *out_buf;
	const void *in_buf;
	size_t out_buf_size;
	size_t out_len;
	size_t in_len;
	int ret;

	in_len = ~0UL;
	out_buf_size = ~0UL;

	switch (argc) {
	case 4:
		out_buf_size = hextoul(argv[3], NULL);
		/* fall through */
	case 3:
		in_buf_addr = hextoul(argv[1], NULL);
		out_buf_addr = hextoul(argv[2], NULL);
		break;
	default:
		return CMD_RET_USAGE;
	}

	out_buf = (void*)out_buf_addr;
	in_buf = (void*)in_buf_addr;

	ret = unxz(in_buf, in_len, &out_len, out_buf, out_buf_size);

	if (ret != UNXZ_OK) {
		printf("Uncompressed fail: ret=%d\n", ret);
		return -EINVAL;
	}

	printf("Uncompressed size: %ld = %#lX\n", (ulong)out_len, (ulong)out_len);
	env_set_hex("filesize", out_len);

	return 0;
}

U_BOOT_CMD(
	xzdec,    4,    1,    do_xzdec,
	"xz uncompress a memory region",
	"srcaddr dstaddr [dstsize]"
);
