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
#include <part.h>
#include <dm.h>
#include <mmc.h>
#include <fs.h>
#include <vsprintf.h>
#include <xyzModem.h>
#include <linux/stringify.h>
#include <linux/types.h>
#include <asm/global_data.h>

#include "mtk_wget.h"
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

	if (env_update("ipaddr", CONFIG_IPADDR,
		       "Input U-Boot's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

	if (env_update("serverip", CONFIG_SERVERIP,
		       "Input TFTP server's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

#ifdef CONFIG_NETMASK
	if (env_update("netmask", CONFIG_NETMASK,
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

#ifdef CONFIG_MTK_WGET
static int load_wget(ulong addr, size_t *data_size, const char *env_name)
{
	char file_name[CONFIG_SYS_CBSIZE + 1];
	size_t size;
	int ret;

	if (env_update("ipaddr", CONFIG_IPADDR,
		       "Input U-Boot's IP address:", NULL, 0))
		return CMD_RET_FAILURE;

#ifdef CONFIG_NETMASK
	if (env_update("netmask", CONFIG_NETMASK,
		       "Input IP netmask:", NULL, 0))
		return CMD_RET_FAILURE;
#endif

	if (env_update(env_name, "", "Input file URL:",
		       file_name, sizeof(file_name)))
		return CMD_RET_FAILURE;

	printf("\n");

	ret = start_wget(addr, file_name, &size);
	if (ret) {
		printf("\n");
		cprintln(ERROR, "*** Wget failed ***");
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
	char saddr[20];
	int repeatable;
	size_t size = 0;
	int ret;

	cprintln(PROMPT, "*** Starting Kermit transmitting ***");
	printf("\n");

	snprintf(saddr, sizeof(saddr),"0x%lx", addr);
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
	char saddr[20];
	int repeatable;
	size_t size = 0;
	int ret;

	cprintln(PROMPT, "*** Starting S-Record transmitting ***");
	printf("\n");

	snprintf(saddr, sizeof(saddr), "0x%lx", addr);
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

#if defined(CONFIG_BLK) && defined(CONFIG_PARTITIONS) && defined(CONFIG_FS_FAT)
static const char *part_get_name(int part_type)
{
	struct part_driver *drv =
		ll_entry_start(struct part_driver, part_driver);
	const int n_ents = ll_entry_count(struct part_driver, part_driver);
	struct part_driver *entry;

	for (entry = drv; entry != drv + n_ents; entry++) {
		if (part_type == entry->part_type)
			return entry->name;
	}

	return "Unknown";
}

static void print_size_str(char *str, uint64_t size)
{
	uint32_t integer, rem, decimal;
	const char *unit;

	if (size >= SZ_1G) {
		integer = size / SZ_1G;
		rem = (size % SZ_1G + SZ_1M / 2) / SZ_1M;
		unit = "G";
	} else if (size >= SZ_1M) {
		integer = size / SZ_1M;
		rem = (size % SZ_1M + SZ_1K / 2) / SZ_1K;
		unit = "M";
	} else if (size >= SZ_1K) {
		integer = size / SZ_1K;
		rem = size % SZ_1K;
		unit = "K";
	} else {
		integer = size;
		rem = 0;
		unit = "";
	}

	decimal = rem * 100 / 1024;

	if (!decimal) {
		/* XX.00 */
		sprintf(str, "%u%sB", integer, unit);
		return;
	}

	if (!(decimal % 10)) {
		/* XX.Y0 */
		sprintf(str, "%u.%u%sB", integer, decimal / 10, unit);
		return;
	}

	/* XX.YY / XX.0Y */
	sprintf(str, "%u.%02u%sB", integer, decimal, unit);
}

static int __maybe_unused load_from_blkdev(struct blk_desc *bd, size_t addr,
					   size_t *data_size,
					   const char *env_name)
{
	char input_buffer[CONFIG_SYS_CBSIZE + 1], start_str[16], size_str[16];
	u32 i, nparts = 0, upartidx = 1;
	loff_t filesize, readsize = 0;
	struct disk_partition dpart;
	char *endp, *filename;
	int ret;

#ifdef CONFIG_BLOCK_CACHE
	blkcache_invalidate(bd->uclass_id, bd->devnum);
#endif

	part_init(bd);

	if (bd->part_type == PART_TYPE_UNKNOWN) {
		cprintln(ERROR, "*** Unrecognized partition type! ***");
		return CMD_RET_FAILURE;
	}

	printf("Partition table type: %s\n", part_get_name(bd->part_type));

	printf("\nAvailable partition(s):\n\n");

#ifdef CONFIG_DOS_PARTITION
	if (bd->part_type == PART_TYPE_DOS)
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
		printf("    #  | Offset   | Size     | UUID        | Type\n");
#else
		printf("    #  | Offset   | Size     | Type\n");
#endif
	else
#endif /* CONFIG_DOS_PARTITION */
	if (bd->part_type == PART_TYPE_EFI)
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
		printf("    #  | Offset   | Size     | UUID                                 | Name\n");
#else
		printf("    #  | Offset   | Size     | Name\n");
#endif
	else
		printf("    #  | Offset   | Size\n");

	i = 1;
	while (true) {
		ret = part_get_info(bd, i, &dpart);
		if (ret)
			break;

		print_size_str(start_str, (uint64_t)dpart.start * dpart.blksz);
		print_size_str(size_str, (uint64_t)dpart.size * dpart.blksz);

#ifdef CONFIG_DOS_PARTITION
		if (bd->part_type == PART_TYPE_DOS) {
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
			printf("    %-2u   %-8s   %-8s   %-11s   %02X   %s\n",
			       i, start_str, size_str, dpart.uuid,
			       dpart.sys_ind, dpart.bootable ? "(Active)" : "");
#else
			printf("    %-2u   %-8s   %-8s   %02X   %s\n",
			       i, start_str, size_str, dpart.sys_ind,
			       dpart.bootable ? "(Active)" : "");
#endif
		} else
#endif /* CONFIG_DOS_PARTITION */
		if (bd->part_type == PART_TYPE_EFI) {
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
			printf("    %-2u   %-8s   %-8s   %s   %s\n",
			       i, start_str, size_str, dpart.uuid, dpart.name);
#else
			printf("    %-2u   %-8s   %-8s   %s\n",
			       i, start_str, size_str, dpart.name);
#endif
		} else {
			printf("    %-2u   %-8s   %-8s\n",
			       i, start_str, size_str);
		}

		i++;
		nparts++;
	}

	printf("\n");

	if (!nparts) {
		cprintln(ERROR, "*** No partition found! ***");
		return CMD_RET_FAILURE;
	}

	if (nparts == 1)
		goto start_fs_load;

	input_buffer[0] = 0;
	cli_highlight_input("Select partition:");
	if (cli_readline_into_buffer(NULL, input_buffer, 0) == -1)
		return CMD_RET_FAILURE;

	if (!input_buffer[0] || input_buffer[0] == '\r' ||
	    input_buffer[0] == '\n')
		goto start_fs_load;

	upartidx = simple_strtoul(input_buffer, &endp, 0);
	if (*endp || endp == input_buffer || upartidx > nparts) {
		cprintln(ERROR, "*** Invalid partition index! ***");
		return CMD_RET_FAILURE;
	}

start_fs_load:
	ret = fs_set_blk_dev_with_part(bd, upartidx);
	if (ret) {
		cprintln(ERROR, "*** Failed to attach Filesystem! ***");
		return CMD_RET_FAILURE;
	}

	printf("Attached to partition %u (%s)\n\n", upartidx,
	       fs_get_type_name());

	if (env_update(env_name, "", "Input file name:",
		       input_buffer + 1, sizeof(input_buffer) - 1))
		return CMD_RET_FAILURE;

	if (input_buffer[1] == '/') {
		filename = input_buffer + 1;
	} else {
		filename = input_buffer;
		input_buffer[0] = '/';
	}

	printf("\n");

	ret = fs_size(filename, &filesize);
	if (ret) {
		cprintln(ERROR, "*** File '%s' not exist! ***", filename);
		return CMD_RET_FAILURE;
	}

	print_size_str(size_str, filesize);
	printf("File size: %s\n", size_str);

	/* This function must be called before every fs_* APIs */
	fs_set_blk_dev_with_part(bd, upartidx);

	ret = fs_read(filename, addr, 0, 0, &readsize);
	if (ret) {
		cprintln(ERROR, "*** Failed to load file! ***");
		return CMD_RET_FAILURE;
	}

	*data_size = readsize;
	return CMD_RET_SUCCESS;
}
#endif

#ifdef CONFIG_MTK_LOAD_FROM_SD
static int load_sd(ulong addr, size_t *data_size, const char *env_name)
{
	struct mmc *mmc;
	int ret = CMD_RET_FAILURE;

	mmc = find_mmc_device(CONFIG_MTK_SD_DEV_IDX);
	if (!mmc) {
		cprintln(ERROR, "*** SD device not found! ***");
		goto err_out;
	}

	mmc->has_init = 0;

	ret = mmc_init(mmc);
	if (ret) {
		cprintln(ERROR, "*** Failed to initialize SD device! ***");
		goto err_out;
	}

	printf("Using device: %s\n", mmc->dev->name);

	ret = load_from_blkdev(mmc_get_blk_desc(mmc), addr, data_size, env_name);
	if (ret == CMD_RET_SUCCESS)
		return ret;

err_out:
	cprintln(ERROR, "*** Operation Aborted! ***");
	return ret;
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
	if (*addr_end || !datasize || datasize >= CONFIG_SYS_BOOTM_LEN) {
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
#ifdef CONFIG_MTK_WGET
	{
		.name = "HTTP Wget",
		.load_func = load_wget
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
#ifdef CONFIG_MTK_LOAD_FROM_SD
	{
		.name = "SD card",
		.load_func = load_sd
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

__weak ulong board_get_load_addr(void)
{
	return CONFIG_SYS_LOAD_ADDR;
}

ulong get_load_addr(void)
{
	char *env_la, *end;
	ulong addr;

	env_la = env_get("loadaddr");
	if (!env_la)
		return board_get_load_addr();

	addr = strtoul(env_la, &end, 16);
	if (!*end && addr >= gd->ram_base && addr < gd->ram_top &&
	    addr != CONFIG_SYS_LOAD_ADDR)
		return addr;

	return board_get_load_addr();
}
