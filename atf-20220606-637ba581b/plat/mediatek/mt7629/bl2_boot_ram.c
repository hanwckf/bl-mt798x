/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <assert.h>
#include <drivers/io/io_driver.h>
#include <drivers/io/io_memmap.h>
#include <plat_private.h>
#include <lib/mmio.h>
#include <bl2_boot_dev.h>
#include <uart_dl.h>
#include <hsuart.h>

#define MT7629_DRAM_BASE		0x40000000

#define FIP_BASE			0x20000
#define FIP_SIZE			0x200000

#define DEBUGGER_HOOK_ADDR		0x100200

const io_block_spec_t mtk_boot_dev_fip_spec = {
	.offset	= MT7629_DRAM_BASE + FIP_BASE,
	.length = FIP_SIZE,
};

#ifdef RAM_BOOT_UART_DL
int uart_dl_api_getc(void)
{
	while (!(mmio_read_32(UART0_BASE + UART_LSR) & UART_LSR_DR))
		;

	return mmio_read_32(UART0_BASE + UART_RBR);
}

void uart_dl_api_putc(int ch)
{
	while (!(mmio_read_32(UART0_BASE + UART_LSR) & UART_LSR_THRE))
		;

	mmio_write_32(UART0_BASE + UART_THR, ch);
}

void uart_dl_api_set_baud(uint32_t baudrate)
{
	uint32_t mask = UART_LSR_THRE | UART_LSR_TEMT;

	if (!baudrate)
		baudrate = UART_BAUDRATE;

	while ((mmio_read_32(UART0_BASE + UART_LSR) & mask) != mask)
		;

	console_hsuart_init(UART0_BASE, plat_uart_clock(), baudrate, true);
}
#endif

void mtk_boot_dev_setup(const io_dev_connector_t **boot_dev_con,
			uintptr_t *boot_dev_handle)
{
	int result;

#ifdef RAM_BOOT_DEBUGGER_HOOK
	/* debugger hook */
	mmio_write_32(DEBUGGER_HOOK_ADDR, 0);

	while (mmio_read_32(DEBUGGER_HOOK_ADDR) == 0)
		;
#endif

#ifdef RAM_BOOT_UART_DL
	/* Disable Watchdog */
	mmio_write_32(MTK_WDT_BASE, MTK_WDT_MODE_KEY);

	start_uart_dl(mtk_boot_dev_fip_spec.offset);
#endif

	result = register_io_dev_memmap(boot_dev_con);
	assert(result == 0);

	result = io_dev_open(*boot_dev_con, (uintptr_t)NULL, boot_dev_handle);
	assert(result == 0);

	/* Ignore improbable errors in release builds */
	(void)result;
}
