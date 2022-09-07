/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _BR_SPI_H_
#define _BR_SPI_H_

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <common/debug.h>
#include <plat/common/platform.h>

#define dma_addr_t	uint32_t

struct spi_transfer {
	uint8_t	*tx_buf;
	uint8_t		*rx_buf;
	unsigned	len;
	dma_addr_t tx_dma;
	dma_addr_t rx_dma;

	uint32_t		speed_hz;
};

/* Board specific platform_data */
struct mtk_chip_config {
	uint32_t sample_sel;
	uint32_t get_tick_dly;
	uint32_t inter_loopback;
};

struct cs_timing {
	/* unit is the period of source clk, range:1~65536 */
	uint32_t cs_setup;
	uint32_t cs_hold;
};

struct spi_device {
	struct mtk_chip_config	*chip_config;
	struct cs_timing *cs_timing;

	uint32_t			src_clk_hz;
	uint32_t			max_speed_hz;
	uint32_t			mode;
#define	SPI_MODE_0	(0|0)
#define	SPI_MODE_1	(0|SPI_CPHA)
#define	SPI_MODE_2	(SPI_CPOL|0)
#define	SPI_MODE_3	(SPI_CPOL|SPI_CPHA)
};

/* Error Code */
#define ERR_SPI_OK			(0)
#define ERR_SPI_INIT		(-1)
#define ERR_SPI_TIMEOUT		(-2)
#define ERR_SPI_PARAM		(-3)

int mtk_spi_init(struct spi_device *spi);

int mtk_spi_fifo_transfer(struct spi_device *spi,
				 struct spi_transfer *xfer);
int mtk_spi_dma_transfer(struct spi_device *spi,
				struct spi_transfer *xfer);

bool mtk_spi_mem_supports_op(struct spi_device *spi,
				     const struct spi_mem_op *op);
int mtk_spi_mem_adjust_op_size(struct spi_device *spi,
				struct spi_mem_op *op);
int mtk_spi_mem_exec_op(struct spi_device *spi,
			 const struct spi_mem_op *op);

int mtk_qspi_init(uint32_t src_clk_hz);

#endif
