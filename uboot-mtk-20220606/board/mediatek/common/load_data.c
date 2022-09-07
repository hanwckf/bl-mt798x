// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2021 MediaTek Inc. All Rights Reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 *
 * Generic data loading helper
 */

#include <stdbool.h>
#include <cli.h>
#include <command.h>
#include <console.h>
#include <cpu_func.h>
#include <env.h>
#include <image.h>
#include <net.h>
#include <vsprintf.h>
#include <xyzModem.h>
#include <linux/stringify.h>
#include <linux/types.h>

#include "load_data.h"
#include "colored_print.h"

#define BUF_SIZE	1024

static void cli_highlight_input(const char *prompt)
{
	cprint(INPUT, "%s", prompt);
	printf(" ");
}

static int env_read_cli_set(const char *varname, const char *defval,
			    const char *prompt, char *buffer, size_t bufsz)
{
	char input_buffer[CONFIG_SYS_CBSIZE + 1], *inbuf;
	char *argv[] = { "env", "set", NULL, NULL, NULL };
	const char *tmpstr = NULL;
	int repeatable = 0;
	size_t inbufsz;

	if (buffer && bufsz) {
		inbuf = buffer;
		inbufsz = bufsz;
	} else {
		inbuf = input_buffer;
		inbufsz = sizeof(input_buffer);
	}

	if (varname)
		tmpstr = env_get(varname);

	if (!tmpstr)
		tmpstr = defval;

	if (strlen(tmpstr) > inbufsz - 1) {
		strncpy(inbuf, tmpstr, inbufsz - 1);
		inbuf[inbufsz - 1] = 0;
	} else {
		strcpy(inbuf, tmpstr);
	}

	cli_highlight_input(prompt);
	if (cli_readline_into_buffer(NULL, inbuf, 0) == -1)
		return -1;

	if (!inbuf[0] || inbuf[0] == 10 || inbuf[0] == 13)
		return 1;

	if (!varname)
		return 0;

	argv[2] = (char *)varname;
	argv[3] = inbuf;

	return cmd_process(0, 4, argv, &repeatable, NULL);
}

int env_update(const char *varname, const char *defval, const char *prompt,
	       char *buffer, size_t bufsz)
{
	while (1) {
		switch (env_read_cli_set(varname, defval, prompt, buffer,
					 bufsz)) {
		case 0:
			return 0;
		case -1:
			printf("\n");
			cprintln(ERROR, "*** Operation Aborted! ***");
			return 1;
		}
	}
}

int confirm_yes(const char *prompt)
{
	char yn[CONFIG_SYS_CBSIZE + 1];

	yn[0] = 0;

	cli_highlight_input(prompt);
	if (cli_readline_into_buffer(NULL, yn, 0) == -1)
		return 0;

	puts("\n");

	if (!yn[0] || yn[0] == '\r' || yn[0] == '\n')
		return 1;

	return !strcmp(yn, "y") || !strcmp(yn, "Y") ||
		!strcmp(yn, "yes") || !strcmp(yn, "YES");
}

#ifdef CONFIG_CMD_TFTPBOOT
static int load_tftp(ulong addr, size_t *data_size, const char *env_name)
{
	char file_name[CONFIG_SYS_CBSIZE + 1];
	u32 size;

	if (env_update("ipaddr", __stringify(CONFIG_IPADDR),
		       "Input U-Boot's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

	if (env_update("serverip", __stringify(CONFIG_SERVERIP),
		       "Input TFTP server's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

#ifdef CONFIG_NETMASK
	if (env_update("netmask", __stringify(CONFIG_NETMASK),
		       "Input IP netmask:", NULL, 0))
		return CMD_RET_FAILURE;
#endif

	if (env_update(env_name, "", "Input file name:",
		       file_name, sizeof(file_name)))
		return CMD_RET_FAILURE;

	printf("\n");

	image_load_addr = addr;
	copy_filename(net_boot_file_name, file_name,
		      sizeof(net_boot_file_name));

	size = net_loop(TFTPGET);
	if ((int)size < 0) {
		printf("\n");
		cprintln(ERROR, "*** TFTP client failure: %d ***", size);
		cprintln(ERROR, "*** Operation Aborted! ***");
		return CMD_RET_FAILURE;
	}

	if (data_size)
		*data_size = size;

	env_save();

	return CMD_RET_SUCCESS;
}
#endif

#ifdef CONFIG_CMD_LOADB
static int getcymodem(void)
{
	if (tstc())
		return (getchar());
	return -1;
}

static int load_xymodem(int mode, ulong addr, size_t *data_size)
{
	connection_info_t info;
	char *buf = (char *)addr;
	size_t size = 0;
	int ret, err;
	char xyc;

	xyc = (mode == xyzModem_xmodem ? 'X' : 'Y');

	cprintln(PROMPT, "*** Starting %cmodem transmitting ***", xyc);
	printf("\n");

	info.mode = mode;
	ret = xyzModem_stream_open(&info, &err);
	if (ret) {
		printf("\n");
		cprintln(ERROR, "*** %cmodem error: %s ***", xyc,
			 xyzModem_error(err));
		cprintln(ERROR, "*** Operation Aborted! ***");
		return CMD_RET_FAILURE;
	}

	while ((ret = xyzModem_stream_read(buf + size, BUF_SIZE, &err)) > 0)
		size += ret;

	xyzModem_stream_close(&err);
	xyzModem_stream_terminate(false, &getcymodem);

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}

static int load_xmodem(ulong addr, size_t *data_size, const char *env_name)
{
	return load_xymodem(xyzModem_xmodem, addr, data_size);
}

static int load_ymodem(ulong addr, size_t *data_size, const char *env_name)
{
	return load_xymodem(xyzModem_ymodem, addr, data_size);
}

static int load_kermit(ulong addr, size_t *data_size, const char *env_name)
{
	char *argv[] = { "loadb", NULL, NULL };
	char saddr[16];
	int repeatable;
	size_t size = 0;
	int ret;

	cprintln(PROMPT, "*** Starting Kermit transmitting ***");
	printf("\n");

	sprintf(saddr, "0x%lx", addr);
	argv[1] = saddr;

	ret = cmd_process(0, 2, argv, &repeatable, NULL);
	if (ret)
		return ret;

	size = env_get_hex("filesize", 0);
	if (!size)
		return CMD_RET_FAILURE;

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}
#endif

#ifdef CONFIG_CMD_LOADS
static int load_srecord(ulong addr, size_t *data_size, const char *env_name)
{
	char *argv[] = { "loads", NULL, NULL };
	char saddr[16];
	int repeatable;
	size_t size = 0;
	int ret;

	cprintln(PROMPT, "*** Starting S-Record transmitting ***");
	printf("\n");

	sprintf(saddr, "0x%lx", addr);
	argv[1] = saddr;

	ret = cmd_process(0, 2, argv, &repeatable, NULL);
	if (ret)
		return ret;

	size = env_get_hex("filesize", 0);
	if (!size)
		return CMD_RET_FAILURE;

	if (data_size)
		*data_size = size;

	return CMD_RET_SUCCESS;
}
#endif

#ifdef CONFIG_MEDIATEK_LOAD_FROM_RAM
static int load_ram(ulong addr, size_t *data_size, const char *env_name)
{
	char input_buf[CONFIG_SYS_CBSIZE + 1];
	ulong dataaddr, datasize;
	char *addr_end;

	cprintln(CAUTION, "*** Please load data to RAM first ***\n");

	if (env_update("dataaddr", "", "Input data address:", input_buf,
		       sizeof(input_buf)))
		return CMD_RET_FAILURE;

	dataaddr = simple_strtoul(input_buf, &addr_end, 0);
	if (*addr_end) {
		cprintln(ERROR, "*** Invalid data address! ***");
		return CMD_RET_FAILURE;
	}

	if (env_update("datasize", "", "Input data size:", input_buf,
		       sizeof(input_buf)))
		return CMD_RET_FAILURE;

	datasize = simple_strtoul(input_buf, &addr_end, 0);
	if (*addr_end || !datasize) {
		cprintln(ERROR, "*** Invalid data size! ***");
		return CMD_RET_FAILURE;
	}

	if (addr != dataaddr)
		memmove((void *)addr, (void *)dataaddr, datasize);

	if (data_size)
		*data_size = datasize;

	return CMD_RET_SUCCESS;
}
#endif

struct load_method {
	const char *name;
	int (*load_func)(ulong addr, size_t *data_size, const char *env_name);
} load_methods[] = {
#ifdef CONFIG_CMD_NET
	{
		.name = "TFTP client",
		.load_func = load_tftp
	},
#endif
#ifdef CONFIG_CMD_LOADB
	{
		.name = "Xmodem",
		.load_func = load_xmodem
	},
	{
		.name = "Ymodem",
		.load_func = load_ymodem
	},
	{
		.name = "Kermit",
		.load_func = load_kermit
	},
#endif
#ifdef CONFIG_CMD_LOADS
	{
		.name = "S-Record",
		.load_func = load_srecord
	},
#endif
#ifdef CONFIG_MEDIATEK_LOAD_FROM_RAM
	{
		.name = "RAM",
		.load_func = load_ram
	},
#endif
};

int load_data(ulong addr, size_t *data_size, const char *env_name)
{
	int i;
	char c;

	cprintln(PROMPT, "Available load methods:");

	for (i = 0; i < ARRAY_SIZE(load_methods); i++) {
		printf("    %d - %s", i, load_methods[i].name);
		if (i == 0)
			printf(" (Default)");
		printf("\n");
	}

	printf("\n");
	cprint(PROMPT, "Select (enter for default):");
	printf(" ");

	c = getchar();
	printf("%c\n\n", c);

	if (c == '\r' || c == '\n')
		c = '0';

	i = c - '0';
	if (i < 0 || i >= ARRAY_SIZE(load_methods)) {
		cprintln(ERROR, "*** Invalid selection! ***");
		return CMD_RET_FAILURE;
	}

	if (load_methods[i].load_func(addr, data_size, env_name))
		return CMD_RET_FAILURE;

	flush_cache(addr, *data_size);

	return CMD_RET_SUCCESS;
}

int load_data_tftp_silent(ulong addr, size_t *data_size, const char *boot_file_name)
{
	u32 size;

	copy_filename(net_boot_file_name, boot_file_name, sizeof(net_boot_file_name));

	size = net_loop(TFTPGET);
	if ((int)size < 0) {
		printf("\n");
		cprintln(ERROR, "*** TFTP client failure: %d ***", size);
		cprintln(ERROR, "*** Operation Aborted! ***");
		return CMD_RET_FAILURE;
	}

	if (data_size)
		*data_size = size;

	flush_cache(addr, *data_size);

	return CMD_RET_SUCCESS;
}
