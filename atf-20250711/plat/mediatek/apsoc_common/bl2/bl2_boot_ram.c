// SPDX-License-Identifier: BSD-3-Clause
/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * Author: Weijie Gao <weijie.gao@mediatek.com>
 */

#include <errno.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_memmap.h>
#include <platform_def.h>
#include <plat_private.h>
#include <mtk_wdt.h>
#include <uart_dl.h>
#include <hsuart.h>
#include "bl2_plat_setup.h"

#define DEBUGGER_HOOK_ADDR		0x100200

static io_block_spec_t mmap_dev_fip_spec = {
	.offset = SCRATCH_BUF_OFFSET,
	.length = SCRATCH_BUF_SIZE,
};

#ifdef RAM_BOOT_UART_DL
int uart_dl_api_getc(void)
{
	return hsuart_getc(UART_BASE);
}

void uart_dl_api_putc(int ch)
{
	hsuart_putc(UART_BASE, ch);
}

void uart_dl_api_set_baud(uint32_t baudrate)
{
	if (!baudrate)
		baudrate = UART_BAUDRATE;

	while (!hsuart_tx_done(UART_BASE))
		;

	hsuart_set_baud(UART_BASE, UART_CLOCK, baudrate, true);
}
#endif

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

#ifndef FPGA_EMU_WDT_TEST
	/* Disable Watchdog */
	mtk_wdt_control(false);
#endif

#ifdef RAM_BOOT_DEBUGGER_HOOK
	/* debugger hook */
	mmio_write_32(DEBUGGER_HOOK_ADDR, 0);

	while (mmio_read_32(DEBUGGER_HOOK_ADDR) == 0)
		;
#endif

#ifdef RAM_BOOT_UART_DL
	start_uart_dl(mmap_dev_fip_spec.offset);
#endif

	ret = register_io_dev_memmap(&dev_con);
	if (ret)
		return ret;

	ret = io_dev_open(dev_con, (uintptr_t)NULL, dev_handle);
	if (ret)
		return ret;

	*image_spec = (uintptr_t)&mmap_dev_fip_spec;

	return 0;
}
