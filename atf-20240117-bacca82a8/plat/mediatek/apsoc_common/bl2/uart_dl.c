/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <common/debug.h>
#include <drivers/console.h>
#include <platform_def.h>
#include <uart_dl.h>

#define DLCMD_GET_VER				0x01
#define DLCMD_SET_BAUD				0x02
#define DLCMD_LOAD_DATA				0x03
#define DLCMD_GO				0x04

#define UARTDL_VER				0x10

enum cmd_proc_result {
	CMD_OK,
	CMD_REPEAT,
	CMD_FAIL
};

static const uint8_t request_cmd[] = { 'm', 'u', 'd', 'l' };
static const uint8_t response_cmd[] = { 'T', 'F', '-', 'A' };

static uintptr_t data_loadaddr;
static size_t data_size;

static void handshake(void)
{
	uint32_t i = 0;
	int ch;

	while (true) {
		ch = uart_dl_api_getc();
		if (ch == ERROR_NO_PENDING_CHAR)
			continue;

		if (ch == request_cmd[i]) {
			uart_dl_api_putc(response_cmd[i]);
			i++;

			if (i >= sizeof(response_cmd))
				break;

			continue;
		}

		i = 0;
	}
}

static uint32_t read_u16(void)
{
	int d0, d1;

	d0 = uart_dl_api_getc();
	d1 = uart_dl_api_getc();

	return ((d0 & 0xff) << 8) | (d1 & 0xff);
}

static uint32_t read_u32(void)
{
	int d0, d1, d2, d3;

	d0 = uart_dl_api_getc();
	d1 = uart_dl_api_getc();
	d2 = uart_dl_api_getc();
	d3 = uart_dl_api_getc();

	return ((d0 & 0xff) << 24) | ((d1 & 0xff) << 16) |
	       ((d2 & 0xff) << 8) | (d3 & 0xff);
}

static void write_u8(uint8_t val)
{
	uart_dl_api_putc(val);
}

static void write_u16(uint16_t val)
{
	uart_dl_api_putc((val >> 8) & 0xff);
	uart_dl_api_putc(val & 0xff);
}

static void write_u32(uint32_t val)
{
	uart_dl_api_putc((val >> 24) & 0xff);
	uart_dl_api_putc((val >> 16) & 0xff);
	uart_dl_api_putc((val >> 8) & 0xff);
	uart_dl_api_putc(val & 0xff);
}

static uint16_t calc_chksum(const uint8_t *data, uint32_t size)
{
	uint16_t val;
	uint32_t chksum = 0;

	while (size >= 2) {
		val = ((uint16_t)data[0] << 8) | data[1];
		chksum += val;
		data += 2;
		size -= 2;
	}

	if (size) {
		val = (uint16_t)data[0] << 8;
		chksum += val;
	}

	while (chksum >> 16)
		chksum = ((chksum >> 16) & 0xffff) + (chksum & 0xffff);

	return chksum;
}

static uint32_t receive_packet(uintptr_t addr, uint32_t expected_idx)
{
	uint8_t *ptr = (uint8_t *)addr;
	uint16_t chksum, real_chksum, len, rcvd_len = 0;
	uint32_t idx;

	idx = read_u32();
	write_u32(idx);

	len = read_u16();
	write_u16(len);

	chksum = read_u16();
	write_u16(chksum);

	while (rcvd_len < len) {
		*ptr++ = uart_dl_api_getc();
		rcvd_len++;
	}

	if (idx != expected_idx) {
		write_u32(expected_idx);
		write_u16(0);
		return 0;
	}

	real_chksum = calc_chksum((uint8_t *)addr, len);
	write_u32(expected_idx);
	write_u16(real_chksum);

	if (real_chksum != chksum)
		return 0;

	return len;
}

static void receive_data(void)
{
	uint32_t curridx = 0, rcvd_size = 0, ret;
	uintptr_t curraddr = data_loadaddr;
	uint32_t size;

	size = read_u32();
	write_u32(size);

	while (rcvd_size < size) {
		ret = receive_packet(curraddr, curridx);
		if (!ret)
			continue;

		curridx++;
		rcvd_size += ret;
		curraddr += ret;
	}

	data_size = size;
}

static enum cmd_proc_result cmd_process(void)
{
	uint32_t baudrate;
	uint8_t cmd;

	while (true) {
		cmd = uart_dl_api_getc();

		switch (cmd) {
		case DLCMD_GET_VER:
			write_u8(cmd);
			write_u8(UARTDL_VER);
			break;

		case DLCMD_SET_BAUD:
			write_u8(cmd);
			baudrate = read_u32();
			write_u32(baudrate);
			uart_dl_api_set_baud(baudrate);
			return CMD_REPEAT;

		case DLCMD_LOAD_DATA:
			write_u8(cmd);
			receive_data();
			break;

		case DLCMD_GO:
			write_u8(cmd);
			return CMD_OK;

		default:
			write_u8(0xff);
			return CMD_FAIL;
		}
	}
}

void start_uart_dl(uintptr_t loadaddr)
{
	enum cmd_proc_result result;

	data_loadaddr = loadaddr;

	NOTICE("Starting UART download handshake ...\n");

	do {
		handshake();
		result = cmd_process();
	} while (result == CMD_REPEAT);

	if (result == CMD_FAIL) {
		/* Currently handle unrecognized command simply */
		while (1)
			;
	}

	NOTICE("Received FIP 0x%zx @ 0x%08lx ...\n", data_size, loadaddr);
}
