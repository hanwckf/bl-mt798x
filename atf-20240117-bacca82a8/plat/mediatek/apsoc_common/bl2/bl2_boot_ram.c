/*
 * Copyright (c) 2023, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
	while (!(mmio_read_32(UART_BASE + UART_LSR) & UART_LSR_DR))
		;

	return mmio_read_32(UART_BASE + UART_RBR);
}

void uart_dl_api_putc(int ch)
{
	while (!(mmio_read_32(UART_BASE + UART_LSR) & UART_LSR_THRE))
		;

	mmio_write_32(UART_BASE + UART_THR, ch);
}

void uart_dl_api_set_baud(uint32_t baudrate)
{
	uint32_t mask = UART_LSR_THRE | UART_LSR_TEMT;

	if (!baudrate)
		baudrate = UART_BAUDRATE;

	while ((mmio_read_32(UART_BASE + UART_LSR) & mask) != mask)
		;

	console_hsuart_init(UART_BASE, UART_CLOCK, baudrate, true);
}
#endif

int mtk_fip_image_setup(uintptr_t *dev_handle, uintptr_t *image_spec)
{
	const io_dev_connector_t *dev_con;
	int ret;

	/* Disable Watchdog */
	mtk_wdt_control(false);

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
