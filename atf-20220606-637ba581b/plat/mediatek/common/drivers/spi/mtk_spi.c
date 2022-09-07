/*
 * Copyright (c) 2021, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <libfdt.h>
#include <common/fdt_wrappers.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include <timer.h>
#include <mempool.h>
#include <drivers/spi_mem.h>
#include <lib/utils_def.h>
#include <errno.h>

#include "mtk_spi.h"

/******************************************************/
/* Platform Resource Configuration */
/******************************************************/

/* buffer size(unit: byte) for DMA used */
//#define MTK_LIMIT_SPI_BUF_SIZE
#if defined(MTK_LIMIT_SPI_BUF_SIZE)
#define SPI_TX_BUF_SIZE			(128)
#else
#define SPI_TX_BUF_SIZE			(5 * 1024)
#endif
#define SPI_RX_BUF_SIZE			(5 * 1024)

#define SPI_DRV_DEBUG_MACRO	0
/******************************************************/
/* SPI register definition */
/******************************************************/
#define SPI_CFG0_REG                      0x0000
#define SPI_CFG1_REG                      0x0004
#define SPI_TX_SRC_REG                    0x0008
#define SPI_RX_DST_REG                    0x000c
#define SPI_TX_DATA_REG                   0x0010
#define SPI_RX_DATA_REG                   0x0014
#define SPI_CMD_REG                       0x0018
#define SPI_STATUS0_REG                   0x001c
#define SPI_STATUS1_REG                   0x0020
#define SPI_PAD_SEL_REG                   0x0024
#define SPI_CFG2_REG                      0x0028
#define SPI_TX_SRC_REG_64                 0x002c
#define SPI_RX_DST_REG_64                 0x0030
#define SPI_CFG3_IPM_REG                  0x0040

#define SPI_CFG0_SCK_HIGH_OFFSET          0
#define SPI_CFG0_SCK_LOW_OFFSET           8
#define SPI_CFG0_CS_HOLD_OFFSET           16
#define SPI_CFG0_CS_SETUP_OFFSET          24
#define SPI_ADJUST_CFG0_CS_HOLD_OFFSET    0
#define SPI_ADJUST_CFG0_CS_SETUP_OFFSET   16

#define SPI_CFG1_CS_IDLE_OFFSET           0
#define SPI_CFG1_PACKET_LOOP_OFFSET       8
#define SPI_CFG1_PACKET_LENGTH_OFFSET     16
#define SPI_CFG1_GET_TICKDLY_OFFSET       29

#define SPI_CFG1_GET_TICKDLY_MASK	  BITS(29, 31)
#define SPI_CFG1_CS_IDLE_MASK             0xff
#define SPI_CFG1_PACKET_LOOP_MASK         0xff00
#define SPI_CFG1_PACKET_LENGTH_MASK       0x3ff0000
#define SPI_CFG1_IPM_PACKET_LENGTH_MASK   BITS(16, 31)
#define SPI_CFG2_SCK_HIGH_OFFSET          0
#define SPI_CFG2_SCK_LOW_OFFSET		  16

#define SPI_CMD_ACT                  BIT(0)
#define SPI_CMD_RESUME               BIT(1)
#define SPI_CMD_RST                  BIT(2)
#define SPI_CMD_PAUSE_EN             BIT(4)
#define SPI_CMD_DEASSERT             BIT(5)
#define SPI_CMD_SAMPLE_SEL           BIT(6)
#define SPI_CMD_CS_POL               BIT(7)
#define SPI_CMD_CPHA                 BIT(8)
#define SPI_CMD_CPOL                 BIT(9)
#define SPI_CMD_RX_DMA               BIT(10)
#define SPI_CMD_TX_DMA               BIT(11)
#define SPI_CMD_TXMSBF               BIT(12)
#define SPI_CMD_RXMSBF               BIT(13)
#define SPI_CMD_RX_ENDIAN            BIT(14)
#define SPI_CMD_TX_ENDIAN            BIT(15)
#define SPI_CMD_FINISH_IE            BIT(16)
#define SPI_CMD_PAUSE_IE             BIT(17)
#define SPI_CMD_IPM_NONIDLE_MODE     BIT(19)
#define SPI_CMD_IPM_SPIM_LOOP        BIT(21)
#define SPI_CMD_IPM_GET_TICKDLY_OFFSET    22

#define SPI_CMD_IPM_GET_TICKDLY_MASK	BITS(22, 24)

#define PIN_MODE_CFG(x)	((x) / 2)

#define SPI_CFG3_IPM_PIN_MODE_OFFSET		0
#define SPI_CFG3_IPM_HALF_DUPLEX_DIR		BIT(2)
#define SPI_CFG3_IPM_HALF_DUPLEX_EN		BIT(3)
#define SPI_CFG3_IPM_XMODE_EN			BIT(4)
#define SPI_CFG3_IPM_NODATA_FLAG		BIT(5)
#define SPI_CFG3_IPM_CMD_BYTELEN_OFFSET		8
#define SPI_CFG3_IPM_ADDR_BYTELEN_OFFSET	12

#define SPI_CFG3_IPM_CMD_PIN_MODE_MASK		BITS(0, 1)
#define SPI_CFG3_IPM_CMD_BYTELEN_MASK		BITS(8, 11)
#define SPI_CFG3_IPM_ADDR_BYTELEN_MASK		BITS(12, 15)

#define MTK_SPI_MAX_FIFO_SIZE 32U
#define MTK_SPI_PACKET_SIZE 1024U
#define MTK_SPI_IPM_PACKET_SIZE (64 * 1024U)
#define MTK_SPI_IPM_PACKET_LOOP (256U)

#define MTK_SPI_32BITS_MASK  (0xffffffff)

#define BITS(m, n)           (~(BIT(m) - 1) & ((BIT(n) - 1) | BIT(n)))

#define readl(addr)          mmio_read_32(addr)
#define writel(val, addr)    mmio_write_32(addr,val)

/******************************************************/
/* structure */
/******************************************************/
struct mtk_spi_compatible {
	/* Must explicitly send dummy Tx bytes to do Rx only transfer */
	bool must_tx;
	/* some IC design adjust cfg register to enhance time accuracy */
	bool enhance_timing;
	/* the IPM IP design improve some feature, and support dual/quad mode */
	bool ipm_design;
	bool support_quad;
};

struct mtk_spi {
	uintptr_t base;
	uint32_t xfer_len;

	uint8_t *tmp_tx_buf;
	uint8_t *tmp_rx_buf;
	dma_addr_t tmp_tx_dma;
	dma_addr_t tmp_rx_dma;

	const struct mtk_spi_compatible *dev_comp;
	struct spi_device *dev;
	bool mem_malloced;
};

/******************************************************/
/* Global Variable Definition */
/******************************************************/
#define DT_QSPI_COMPAT_MTK		"mediatek,ipm-spi"

static const struct mtk_spi_compatible ipm_compat = {
	true,	//must_tx
	true,	//enhance_timing
	true,	//ipm_design
	true,	//support_quad
};

/*
 * A piece of default chip info unless the platform
 * supplies it.
 */
static struct mtk_chip_config mtk_default_chip_info = {
	0,	//sample_sel
	0,	//get_tick_dly
	0,	//inter_loopback
};

static struct spi_device spidev;
static struct mtk_spi g_mdata;

static uint32_t *SPI_TXDMA_BUF;
static uint32_t *SPI_RXDMA_BUF;

/******************************************************/
/* Function */
/******************************************************/
#if SPI_DRV_DEBUG_MACRO
int mtk_spi_log(const char *fmt, ...)
{
	const char *prefix_str;
	int log_level = LOG_LEVEL_NOTICE;
	va_list ap;
	int ret;

	if (log_level > LOG_LEVEL)
		return 0;

	prefix_str = plat_log_get_prefix(log_level);

	while (*prefix_str != '\0') {
		(void)putchar(*prefix_str);
		prefix_str++;
	}

	printf("SPI-BUS: ");

	va_start(ap, fmt);
	ret = vprintf(fmt, ap);
	va_end(ap);

	return ret;
}
#else
int mtk_spi_log(const char *fmt, ...)
{
	return 0;
}
#endif
static struct mtk_spi *mtk_spi_get_bus(void)
{
	return &g_mdata;
}

static void ioread32_rep(uint32_t addr, void *buf, int len)
{
	int i;
	uint32_t *p = (uint32_t *) buf;

	for (i = 0; i < len; i++)
		p[i] = readl(addr);
}

static void iowrite32_rep(uint32_t addr, void *buf, int len)
{
	int i;
	uint32_t *p = (uint32_t *) buf;

	for (i = 0; i < len; i++)
		writel(p[i], addr);
}

static uint32_t vaddr_to_paddr(void *buf)
{
	return (uint64_t)buf;
}

static void dump_data(char *name, uint8_t *data, int size)
{
	int i;

	mtk_spi_log("%s:\n", name);
	for (i = 0; i < size; i++)
		mtk_spi_log("data[%d]=0x%x\n", i, data[i]);
	mtk_spi_log("\n");
}

static void mtk_spi_dump_reg(struct mtk_spi *mdata)
{
	mtk_spi_log("SPI_CFG0_REG=0x%x\n", readl(mdata->base + SPI_CFG0_REG));
	mtk_spi_log("SPI_CFG1_REG=0x%x\n", readl(mdata->base + SPI_CFG1_REG));
	mtk_spi_log("SPI_TX_SRC_REG=0x%x\n", readl(mdata->base + SPI_TX_SRC_REG));
	mtk_spi_log("SPI_RX_DST_REG=0x%x\n", readl(mdata->base + SPI_RX_DST_REG));
	mtk_spi_log("SPI_CMD_REG=0x%x\n", readl(mdata->base + SPI_CMD_REG));
	mtk_spi_log("SPI_STATUS1_REG=0x%x\n", readl(mdata->base + SPI_STATUS1_REG));
	mtk_spi_log("SPI_CFG2_REG=0x%x\n", readl(mdata->base + SPI_CFG2_REG));
	mtk_spi_log("SPI_CFG3_IPM_REG=0x%x\n", readl(mdata->base + SPI_CFG3_IPM_REG));
}

static uint64_t mtk_spi_get_timeout_ms(int speed_hz, int len)
{
	uint64_t ms;

	ms = (8LL * 1000LL * len) / speed_hz;
	ms += ms + 1000; /* 1s tolerance */

	return ms;
}

static inline bool timer_is_timeout(uint64_t start_tick,
                                    uint64_t timeout_tick)
{
        return get_ticks() > start_tick + timeout_tick;
}

static int mtk_spi_wait_for_transfer_done(struct mtk_spi *mdata, uint32_t timeout_ms)
{
	int ret;
	uint32_t irq_status;
	uint64_t time_start, tmo;

	time_start = get_ticks();
	tmo = time_to_tick((timeout_ms * 1000));

	/* polling if SPI HW xfer done until timeout */
	ret = ERR_SPI_TIMEOUT;
	while(!timer_is_timeout(time_start, tmo))
	{
		irq_status = readl(mdata->base + SPI_STATUS1_REG);
		if (irq_status & 0x1) {
			ret = ERR_SPI_OK;
			break;
		}
	}

	return ret;
}

static void mtk_spi_reset(struct mtk_spi *mdata)
{
	uint32_t reg_val;

	/* set the software reset bit in SPI_CMD_REG. */
	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val |= SPI_CMD_RST;
	writel(reg_val, mdata->base + SPI_CMD_REG);

	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val &= ~SPI_CMD_RST;
	writel(reg_val, mdata->base + SPI_CMD_REG);
}

static void mtk_spi_hw_init(struct mtk_spi *mdata,
			   struct spi_device *spi)
{
	struct mtk_chip_config *chip_config = spi->chip_config;
	uint16_t cpha, cpol;
	uint32_t reg_val;

	cpha = spi->mode & SPI_CPHA ? 1 : 0;
	cpol = spi->mode & SPI_CPOL ? 1 : 0;

	if (mdata->dev_comp->enhance_timing) {
		if (mdata->dev_comp->ipm_design) {
			/* CFG3 reg only used for spi-mem,
			 * here write to default value
			 */
			writel(0x0, mdata->base + SPI_CFG3_IPM_REG);

			reg_val = readl(mdata->base + SPI_CMD_REG);
			reg_val &= ~SPI_CMD_IPM_GET_TICKDLY_MASK;
			reg_val |= (chip_config->get_tick_dly & 0x7)
				   << SPI_CMD_IPM_GET_TICKDLY_OFFSET;
			writel(reg_val, mdata->base + SPI_CMD_REG);
		} else {
			reg_val = readl(mdata->base + SPI_CFG1_REG);
			reg_val &= ~SPI_CFG1_GET_TICKDLY_MASK;
			reg_val |= (chip_config->get_tick_dly & 0x7)
				   << SPI_CFG1_GET_TICKDLY_OFFSET;
			writel(reg_val, mdata->base + SPI_CFG1_REG);
		}
	}

	reg_val = readl(mdata->base + SPI_CMD_REG);
	if (mdata->dev_comp->ipm_design) {
		/* SPI transfer without idle time until packet length done */
		 reg_val |= SPI_CMD_IPM_NONIDLE_MODE;

		if (chip_config->inter_loopback)
			reg_val |= SPI_CMD_IPM_SPIM_LOOP;
		else
			reg_val &= ~SPI_CMD_IPM_SPIM_LOOP;
	}

	if (cpha)
		reg_val |= SPI_CMD_CPHA;
	else
		reg_val &= ~SPI_CMD_CPHA;
	if (cpol)
		reg_val |= SPI_CMD_CPOL;
	else
		reg_val &= ~SPI_CMD_CPOL;

	/* set the mlsbx and mlsbtx */
	if (spi->mode & SPI_LSB_FIRST) {
		reg_val &= ~SPI_CMD_TXMSBF;
		reg_val &= ~SPI_CMD_RXMSBF;
	} else {
		reg_val |= SPI_CMD_TXMSBF;
		reg_val |= SPI_CMD_RXMSBF;
	}

	/* set the tx/rx endian */
	reg_val &= ~SPI_CMD_TX_ENDIAN;
	reg_val &= ~SPI_CMD_RX_ENDIAN;

	if (mdata->dev_comp->enhance_timing) {
		/* set CS polarity */
		if (spi->mode & SPI_CS_HIGH)
			reg_val |= SPI_CMD_CS_POL;
		else
			reg_val &= ~SPI_CMD_CS_POL;

		if (chip_config->sample_sel)
			reg_val |= SPI_CMD_SAMPLE_SEL;
		else
			reg_val &= ~SPI_CMD_SAMPLE_SEL;
	}

	/* set finish and pause interrupt always disable */
	reg_val &= ~(SPI_CMD_FINISH_IE | SPI_CMD_PAUSE_IE);

	/* disable dma mode */
	reg_val &= ~(SPI_CMD_TX_DMA | SPI_CMD_RX_DMA);

	/* disable deassert mode */
	reg_val &= ~SPI_CMD_DEASSERT;

	writel(reg_val, mdata->base + SPI_CMD_REG);
}

static void mtk_spi_prepare_transfer(struct mtk_spi *mdata,
				     uint32_t speed_hz)
{
	uint32_t spi_clk_hz, div, sck_time, cs_time, reg_val, cs_setup, cs_hold;
	struct spi_device *spi = mdata->dev;
	struct cs_timing * cs_timing = spi->cs_timing;

	spi_clk_hz = spi->src_clk_hz;
	if (speed_hz < spi_clk_hz / 4)
		div = div_round_up(spi_clk_hz, speed_hz);
	else
		div = 3;

	sck_time = (div + 1) / 2;
	cs_time = sck_time * 2;

	if (cs_timing) {
		cs_setup = cs_timing->cs_setup;
		cs_hold = cs_timing->cs_hold;

		// check arg valid
		if (cs_setup == 0 || cs_setup > 65536)
			cs_setup = cs_time;
		if (cs_hold == 0 || cs_hold > 65536)
			cs_hold = cs_time;
	} else {
		cs_setup = cs_time;
		cs_hold = cs_time;
	}

	if (mdata->dev_comp->enhance_timing) {
		reg_val = (((sck_time - 1) & 0xffff)
			   << SPI_CFG2_SCK_HIGH_OFFSET);
		reg_val |= (((sck_time - 1) & 0xffff)
			   << SPI_CFG2_SCK_LOW_OFFSET);
		writel(reg_val, mdata->base + SPI_CFG2_REG);
		reg_val = (((cs_hold - 1) & 0xffff)
			   << SPI_ADJUST_CFG0_CS_HOLD_OFFSET);
		reg_val |= (((cs_setup - 1) & 0xffff)
			   << SPI_ADJUST_CFG0_CS_SETUP_OFFSET);
		writel(reg_val, mdata->base + SPI_CFG0_REG);
	} else {
		reg_val = (((sck_time - 1) & 0xff)
			   << SPI_CFG0_SCK_HIGH_OFFSET);
		reg_val |= (((sck_time - 1) & 0xff) << SPI_CFG0_SCK_LOW_OFFSET);
		reg_val |= (((cs_hold - 1) & 0xff) << SPI_CFG0_CS_HOLD_OFFSET);
		reg_val |= (((cs_setup - 1) & 0xff) << SPI_CFG0_CS_SETUP_OFFSET);
		writel(reg_val, mdata->base + SPI_CFG0_REG);
	}

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	reg_val &= ~SPI_CFG1_CS_IDLE_MASK;
	reg_val |= (((cs_time - 1) & 0xff) << SPI_CFG1_CS_IDLE_OFFSET);
	writel(reg_val, mdata->base + SPI_CFG1_REG);
}

static void mtk_spi_setup_packet(struct mtk_spi *mdata)
{
	uint32_t packet_size, packet_loop, reg_val;

	if (mdata->dev_comp->ipm_design)
		packet_size = MIN(mdata->xfer_len, MTK_SPI_IPM_PACKET_SIZE);
	else
		packet_size = MIN(mdata->xfer_len, MTK_SPI_PACKET_SIZE);

	packet_loop = mdata->xfer_len / packet_size;

	reg_val = readl(mdata->base + SPI_CFG1_REG);
	if (mdata->dev_comp->ipm_design)
		reg_val &= ~SPI_CFG1_IPM_PACKET_LENGTH_MASK;
	else
		reg_val &= ~SPI_CFG1_PACKET_LENGTH_MASK;
	reg_val |= (packet_size - 1) << SPI_CFG1_PACKET_LENGTH_OFFSET;
	reg_val &= ~SPI_CFG1_PACKET_LOOP_MASK;
	reg_val |= (packet_loop - 1) << SPI_CFG1_PACKET_LOOP_OFFSET;
	writel(reg_val, mdata->base + SPI_CFG1_REG);
}

static void mtk_spi_enable_transfer(struct mtk_spi *mdata)
{
	uint32_t cmd;

	cmd = readl(mdata->base + SPI_CMD_REG);
	cmd |= SPI_CMD_ACT;
	writel(cmd, mdata->base + SPI_CMD_REG);
}

static void mtk_spi_setup_dma_addr(struct mtk_spi *mdata,
				   struct spi_transfer *xfer)
{
	writel((uint32_t)(mdata->tmp_tx_dma & MTK_SPI_32BITS_MASK),
		       mdata->base + SPI_TX_SRC_REG);
	if (xfer->rx_buf)
		writel((uint32_t)(mdata->tmp_rx_dma & MTK_SPI_32BITS_MASK),
		       mdata->base + SPI_RX_DST_REG);
}

int mtk_spi_fifo_transfer(struct spi_device *spi,
				 struct spi_transfer *xfer)
{
	struct mtk_spi *mdata = mtk_spi_get_bus();
	int cnt, remainder, ret = ERR_SPI_OK;
	uint64_t timeout_ms;
	uint32_t reg_val;

	if (xfer->len > MTK_SPI_MAX_FIFO_SIZE) {
		ERROR("[%s]xfer len(%d) invalid\n", __func__, xfer->len);
		return -ERR_SPI_PARAM;
	}

	if (!xfer->tx_buf && !xfer->rx_buf) {
		ERROR("[%s]tx_buf/rx_buf invalid\n", __func__);
		return -ERR_SPI_PARAM;
	}

	mtk_spi_reset(mdata);
	mtk_spi_hw_init(mdata, spi);
	if (xfer->speed_hz > spi->max_speed_hz)
		xfer->speed_hz = spi->max_speed_hz;
	if (xfer->speed_hz == 0)
		xfer->speed_hz = 1 * 1000 * 1000; /* default use 1Mhz */
	mtk_spi_prepare_transfer(mdata, xfer->speed_hz);

	mdata->xfer_len = MIN(MTK_SPI_MAX_FIFO_SIZE, xfer->len);
	mtk_spi_setup_packet(mdata);

	cnt = xfer->len / 4;

	if (xfer->tx_buf) {
		iowrite32_rep(mdata->base + SPI_TX_DATA_REG, xfer->tx_buf, cnt);
	} else {
		mdata->tmp_tx_buf = (uint8_t *) SPI_TXDMA_BUF;
		memset(mdata->tmp_tx_buf, 0, xfer->len);
		iowrite32_rep(mdata->base + SPI_TX_DATA_REG, mdata->tmp_tx_buf, cnt);
	}

	remainder = xfer->len % 4;
	if (remainder > 0) {
		reg_val = 0;
		if (xfer->tx_buf)
			memcpy(&reg_val, xfer->tx_buf + (cnt * 4), remainder);
		else
			memcpy(&reg_val, mdata->tmp_tx_buf + (cnt * 4), remainder);
		writel(reg_val, mdata->base + SPI_TX_DATA_REG);
	}

	mtk_spi_log("***********dump reg before spi transfer***********\n");
	mtk_spi_dump_reg(mdata);
	mtk_spi_log("**********************\n");

	mtk_spi_enable_transfer(mdata);

	timeout_ms = mtk_spi_get_timeout_ms(xfer->speed_hz, xfer->len);
	ret = mtk_spi_wait_for_transfer_done(mdata, timeout_ms);
	if (ret) {
		ERROR("%s:mtk_spi_wait_for_transfer_done timeout\n", __func__);
		mtk_spi_dump_reg(mdata);
		return ret;
	}

	if (xfer->rx_buf) {
		cnt = mdata->xfer_len / 4;
		ioread32_rep(mdata->base + SPI_RX_DATA_REG,
				 xfer->rx_buf, cnt);
		remainder = mdata->xfer_len % 4;
		if (remainder > 0) {
			reg_val = readl(mdata->base + SPI_RX_DATA_REG);
			memcpy(xfer->rx_buf +
				(cnt * 4),
				&reg_val,
				remainder);
		}
	}

	mtk_spi_log("***********dump reg after spi transfer***********\n");
	mtk_spi_dump_reg(mdata);
	mtk_spi_log("**********************\n");

	return ret;
}

int mtk_spi_dma_transfer(struct spi_device *spi,
				struct spi_transfer *xfer)
{
	struct mtk_spi *mdata = mtk_spi_get_bus();
	int ret = ERR_SPI_OK;
	uint64_t timeout_ms;
	uint32_t cmd;

	mtk_spi_reset(mdata);
	mtk_spi_hw_init(mdata, spi);

	if (xfer->speed_hz > spi->max_speed_hz)
		xfer->speed_hz = spi->max_speed_hz;
	if (xfer->speed_hz == 0)
		xfer->speed_hz = 1 * 1000 * 1000; /* default use 1Mhz */
	mtk_spi_prepare_transfer(mdata, xfer->speed_hz);

	cmd = readl(mdata->base + SPI_CMD_REG);
	/* Must explicitly send dummy Tx bytes to do Rx only transfer */
	cmd |= SPI_CMD_TX_DMA;
	if (xfer->rx_buf)
		cmd |= SPI_CMD_RX_DMA;
	writel(cmd, mdata->base + SPI_CMD_REG);

	mdata->xfer_len = xfer->len;
	mtk_spi_setup_packet(mdata);

	/* must do tx */
	if (xfer->tx_buf)
		memcpy((uint8_t *) SPI_TXDMA_BUF, xfer->tx_buf, xfer->len);
	else
		memset((uint8_t *)SPI_TXDMA_BUF, 0, xfer->len);
	mdata->tmp_tx_dma = vaddr_to_paddr(SPI_TXDMA_BUF);
	//DCache_Flush_Invalidate_Range((uint32_t)SPI_TXDMA_BUF, xfer->len);

	if (xfer->rx_buf) {
		memset((uint8_t *)SPI_RXDMA_BUF, 0, xfer->len);
		mdata->tmp_rx_dma = vaddr_to_paddr(SPI_RXDMA_BUF);
		//DCache_Flush_Invalidate_Range((uint32_t)SPI_RXDMA_BUF, xfer->len);
	}

	mtk_spi_setup_dma_addr(mdata, xfer);

	mtk_spi_log("***********dump reg before spi transfer***********\n");
	mtk_spi_dump_reg(mdata);
	mtk_spi_log("**********************\n");

	mtk_spi_enable_transfer(mdata);

	timeout_ms = mtk_spi_get_timeout_ms(xfer->speed_hz, xfer->len);
	ret = mtk_spi_wait_for_transfer_done(mdata, timeout_ms);
	if (ret) {
		ERROR("%s:mtk_spi_wait_for_transfer_done timeout\n", __func__);
		mtk_spi_dump_reg(mdata);
		return ret;
	}

	/* spi disable dma */
	cmd = readl(mdata->base + SPI_CMD_REG);
	cmd &= ~SPI_CMD_TX_DMA;
	if (xfer->rx_buf)
		cmd &= ~SPI_CMD_RX_DMA;
	writel(cmd, mdata->base + SPI_CMD_REG);

	if (xfer->rx_buf) {
		//DCache_Invalidate_Range((uint32_t)SPI_RXDMA_BUF, xfer->len);
		memcpy(xfer->rx_buf, (uint8_t *) SPI_RXDMA_BUF, xfer->len);
	}

	mtk_spi_log("***********dump reg after spi transfer***********\n");
	mtk_spi_dump_reg(mdata);
	mtk_spi_log("**********************\n");

	return ret;
}

static bool op_is_data_dir_in(const struct spi_mem_op *op)
{
	if (op->data.dir == SPI_MEM_DATA_IN && op->data.nbytes > 0)
		return true;
	else
		return false;
}

static void mtk_spi_dump_mem_op(const struct spi_mem_op *op)
{
	mtk_spi_log("Dump spi_mem_op\n");
	mtk_spi_log("cmd:buswidth[%d],opcode[0x%x]\n",
		op->cmd.buswidth, op->cmd.opcode);
	mtk_spi_log("addr:nbytes[%d],buswidth[%d],val[0x%x]\n",
		op->addr.nbytes, op->addr.buswidth, op->addr.val);
	mtk_spi_log("dummy:nbytes[%d],buswidth[%d]\n",
		op->dummy.nbytes, op->dummy.buswidth);
	mtk_spi_log("data:buswidth[%d],dir[%d],nbytes[%d]\n",
		op->data.buswidth, op->data.dir, op->data.nbytes);
	if (op_is_data_dir_in(op))
		mtk_spi_log("data:buf in[0x%x]\n", op->data.buf);
	else if (op->data.dir == SPI_MEM_DATA_OUT)
		mtk_spi_log("data:buf out[0x%x]\n", op->data.buf);
}

bool mtk_spi_mem_supports_op(struct spi_device *spi,
				     const struct spi_mem_op *op)
{
	if (op->data.buswidth > 4 || op->addr.buswidth > 4 ||
	    op->dummy.buswidth > 4 || op->cmd.buswidth > 4) {
		ERROR("[%s]buswidth invalid\n", __func__);
		mtk_spi_dump_mem_op(op);
		return false;
	}

	if (op->addr.nbytes && op->dummy.nbytes &&
	    op->addr.buswidth != op->dummy.buswidth) {
		ERROR("[%s]not supported addr/dummy type\n", __func__);
		mtk_spi_dump_mem_op(op);
		return false;
	}

	if (op->addr.nbytes + op->dummy.nbytes > 16) {
		ERROR("[%s]not supported addr/dummy nbytes\n", __func__);
		mtk_spi_dump_mem_op(op);
		return false;
	}

	if (op->data.nbytes > MTK_SPI_IPM_PACKET_SIZE) {
		if (op->data.nbytes / MTK_SPI_IPM_PACKET_SIZE >
		    MTK_SPI_IPM_PACKET_LOOP ||
		    op->data.nbytes % MTK_SPI_IPM_PACKET_SIZE != 0) {
			ERROR("[%s]packet size/loop not supported\n", __func__);
			mtk_spi_dump_mem_op(op);
			return false;
		}
	}

/*
	if (op->data.dir == SPI_MEM_DATA_IN &&
	    !IS_ALIGNED((uint32_t)op->data.buf, 4))
		return false;
*/

	if (op_is_data_dir_in(op)) {
		if (1 + op->addr.nbytes + op->dummy.nbytes + op->data.nbytes > SPI_RX_BUF_SIZE) {
			ERROR("[%s]rx: nbytes > %d\n", __func__, SPI_RX_BUF_SIZE);
			mtk_spi_dump_mem_op(op);
			return false;
		}
	} else {
		if (1 + op->addr.nbytes + op->dummy.nbytes + op->data.nbytes > SPI_TX_BUF_SIZE) {
			ERROR("[%s]tx: nbytes > %d\n", __func__, SPI_TX_BUF_SIZE);
			mtk_spi_dump_mem_op(op);
			return false;
		}
	}

	return true;
}

int mtk_spi_mem_adjust_op_size(struct spi_device *spi,
				struct spi_mem_op *op)
{
	int opcode_len;

	opcode_len = 1 + op->addr.nbytes + op->dummy.nbytes;
	if ((op->data.dir == SPI_MEM_DATA_IN) &&
		(op->data.nbytes > SPI_RX_BUF_SIZE))
		op->data.nbytes = SPI_RX_BUF_SIZE;
	else if ((op->data.dir == SPI_MEM_DATA_OUT) &&
		(opcode_len + op->data.nbytes > SPI_TX_BUF_SIZE))
		op->data.nbytes = SPI_TX_BUF_SIZE - opcode_len;

	return 0;
}

static void mtk_spi_mem_setup_dma_xfer(struct mtk_spi *mdata,
				   const struct spi_mem_op *op)
{
	writel((uint32_t)(mdata->tmp_tx_dma & MTK_SPI_32BITS_MASK),
	       mdata->base + SPI_TX_SRC_REG);

	if (op_is_data_dir_in(op))
		writel((uint32_t)(mdata->tmp_rx_dma & MTK_SPI_32BITS_MASK),
			   mdata->base + SPI_RX_DST_REG);
}

int mtk_spi_mem_exec_op(struct spi_device *spi,
				const struct spi_mem_op *op)
{
	struct mtk_spi *mdata = mtk_spi_get_bus();
	uint32_t reg_val, nio = 1, tx_size;
	char *tx_tmp_buf;
	int ret = ERR_SPI_OK;
	uint64_t timeout_ms;

	mtk_spi_reset(mdata);
	mtk_spi_hw_init(mdata, spi);
	mtk_spi_prepare_transfer(mdata, spi->max_speed_hz);

	reg_val = readl(mdata->base + SPI_CFG3_IPM_REG);
	/* opcode byte len */
	reg_val &= ~SPI_CFG3_IPM_CMD_BYTELEN_MASK;
	reg_val |= 1 << SPI_CFG3_IPM_CMD_BYTELEN_OFFSET;

	/* addr & dummy byte len */
	reg_val &= ~SPI_CFG3_IPM_ADDR_BYTELEN_MASK;
	if (op->addr.nbytes || op->dummy.nbytes)
		reg_val |= (op->addr.nbytes + op->dummy.nbytes) <<
			    SPI_CFG3_IPM_ADDR_BYTELEN_OFFSET;

	/* data byte len */
	//if (op->data.dir == SPI_MEM_NO_DATA) {
	if (op->data.nbytes == 0) {
		reg_val |= SPI_CFG3_IPM_NODATA_FLAG;
		writel(0, mdata->base + SPI_CFG1_REG);
	} else {
		reg_val &= ~SPI_CFG3_IPM_NODATA_FLAG;
		mdata->xfer_len = op->data.nbytes;
		mtk_spi_setup_packet(mdata);
	}

	if (op->addr.nbytes || op->dummy.nbytes) {
		if (op->addr.buswidth == 1 || op->dummy.buswidth == 1)
			reg_val |= SPI_CFG3_IPM_XMODE_EN;
		else
			reg_val &= ~SPI_CFG3_IPM_XMODE_EN;
	}

	if (op->addr.buswidth == 2 || op->dummy.buswidth == 2 || op->data.buswidth == 2)
		nio = 2;
	else if (op->addr.buswidth == 4 || op->dummy.buswidth == 4 || op->data.buswidth == 4)
		nio = 4;

	reg_val &= ~SPI_CFG3_IPM_CMD_PIN_MODE_MASK;
	reg_val |= PIN_MODE_CFG(nio) << SPI_CFG3_IPM_PIN_MODE_OFFSET;

	reg_val |= SPI_CFG3_IPM_HALF_DUPLEX_EN;
	if (op_is_data_dir_in(op))
		reg_val |= SPI_CFG3_IPM_HALF_DUPLEX_DIR;
	else
		reg_val &= ~SPI_CFG3_IPM_HALF_DUPLEX_DIR;
	writel(reg_val, mdata->base + SPI_CFG3_IPM_REG);

	tx_size = 1 + op->addr.nbytes + op->dummy.nbytes;
	if (op->data.dir == SPI_MEM_DATA_OUT)
		tx_size += op->data.nbytes;

	tx_size = MAX(tx_size, 32U);

	tx_tmp_buf = (char *)SPI_TXDMA_BUF;

	tx_tmp_buf[0] = op->cmd.opcode;

	if (op->addr.nbytes) {
		int i;

		for (i = 0; i < op->addr.nbytes; i++)
			tx_tmp_buf[i + 1] = op->addr.val >>
					(8 * (op->addr.nbytes - i - 1));
	}

	if (op->dummy.nbytes)
		memset(tx_tmp_buf + op->addr.nbytes + 1,
		       0xff,
		       op->dummy.nbytes);

	if (op->data.nbytes && op->data.dir == SPI_MEM_DATA_OUT)
		memcpy(tx_tmp_buf + op->dummy.nbytes + op->addr.nbytes + 1,
		       op->data.buf,
		       op->data.nbytes);

	mdata->tmp_tx_dma = vaddr_to_paddr(tx_tmp_buf);
	//DCache_Flush_Invalidate_Range((uint32_t)SPI_TXDMA_BUF, tx_size);

	if (op_is_data_dir_in(op)) {
		mdata->tmp_rx_dma = vaddr_to_paddr(SPI_RXDMA_BUF);
		//DCache_Flush_Invalidate_Range(mdata->tmp_rx_dma, op->data.nbytes);
	}

	reg_val = readl(mdata->base + SPI_CMD_REG);
	reg_val |= SPI_CMD_TX_DMA;
	if (op_is_data_dir_in(op))
		reg_val |= SPI_CMD_RX_DMA;
	writel(reg_val, mdata->base + SPI_CMD_REG);

	mtk_spi_mem_setup_dma_xfer(mdata, op);

	mtk_spi_log("***********dump op before spi_mem transfer***********\n");
	mtk_spi_dump_mem_op(op);
	mtk_spi_log("***********dump reg before spi_mem transfer***********\n");
	mtk_spi_dump_reg(mdata);
	mtk_spi_log("**********************\n");

	mtk_spi_enable_transfer(mdata);

	timeout_ms = mtk_spi_get_timeout_ms(spi->max_speed_hz, op->data.nbytes);
	/* Wait for the interrupt. */
	ret = mtk_spi_wait_for_transfer_done(mdata, timeout_ms);
	if (ret) {
		ERROR("%s:mtk_spi_wait_for_transfer_done timeout\n", __func__);
		mtk_spi_dump_reg(mdata);
		return ret;
	}

	 /* spi disable dma */
	 reg_val = readl(mdata->base + SPI_CMD_REG);
	 reg_val &= ~SPI_CMD_TX_DMA;
	 if (op_is_data_dir_in(op))
		 reg_val &= ~SPI_CMD_RX_DMA;
	 writel(reg_val, mdata->base + SPI_CMD_REG);

	if (op_is_data_dir_in(op)) {
		//DCache_Invalidate_Range(mdata->tmp_rx_dma, op->data.nbytes);
		memcpy(op->data.buf, (uint8_t *) SPI_RXDMA_BUF, op->data.nbytes);
		dump_data("SPI_RXDMA_BUF", (uint8_t *)SPI_RXDMA_BUF, op->data.nbytes);
		dump_data("op->data.buf", (uint8_t *)op->data.buf, op->data.nbytes);
	}

	mtk_spi_log("***********dump reg after spi_mem transfer***********\n");
	mtk_spi_dump_reg(mdata);
	mtk_spi_log("**********************\n");

	return ret;
}

static int mtk_qspi_claim_bus(unsigned int cs)
{
	return 0;
}

static void mtk_qspi_release_bus(void)
{
}

static int mtk_qspi_set_speed(unsigned int hz)
{
	spidev.max_speed_hz = hz;
	return 0;
}

static int mtk_qspi_set_mode(unsigned int mode)
{
	spidev.mode = mode;
	return 0;
}

static int mtk_qspi_exec_op(const struct spi_mem_op *op)
{
	int ret;

	mtk_spi_log("op cmd:%x mode:%d.%d.%d.%d addr:%llx len:%x\n",
		op->cmd.opcode, op->cmd.buswidth, op->addr.buswidth,
		op->dummy.buswidth, op->data.buswidth,
		op->addr.val, op->data.nbytes);

	if (!mtk_spi_mem_supports_op(&spidev, op))
		return -1;

	ret = mtk_spi_mem_exec_op(&spidev, op);
	if (ret)
		return ret;

	return 0;
}

static const struct spi_bus_ops mtk_qspi_bus_ops = {
	.claim_bus = mtk_qspi_claim_bus,
	.release_bus = mtk_qspi_release_bus,
	.set_speed = mtk_qspi_set_speed,
	.set_mode = mtk_qspi_set_mode,
	.exec_op = mtk_qspi_exec_op,
};

extern uint8_t dtb_data;
extern uint32_t dtb_data_size;
static void *fdt = &dtb_data;

int mtk_qspi_init(uint32_t src_clk_hz)
{
	int qspi_node;
	int qspi_subnode = 0;
	const fdt32_t *cuint;

	qspi_node = fdt_node_offset_by_compatible(fdt, -1, DT_QSPI_COMPAT_MTK);
	if (qspi_node < 0) {
		ERROR("%s find qspi_node fail\n", __func__);
		return -FDT_ERR_NOTFOUND;
	}

	memset(&g_mdata, 0, sizeof(struct mtk_spi));
	while (qspi_node != -FDT_ERR_NOTFOUND) {
		cuint = fdt_getprop(fdt, qspi_node, "reg", NULL);
		if (cuint) {
			g_mdata.base = fdt32_to_cpu(*cuint);
			break;
		}
	}

	spidev.chip_config = &mtk_default_chip_info;
	fdt_for_each_subnode(qspi_subnode, fdt, qspi_node) {
		cuint = fdt_getprop(fdt, qspi_subnode, "sample-sel", NULL);
		if (cuint)
			spidev.chip_config->sample_sel = fdt32_to_cpu(*cuint);

		cuint = fdt_getprop(fdt, qspi_subnode, "tick-dly", NULL);
		if (cuint)
			spidev.chip_config->get_tick_dly = fdt32_to_cpu(*cuint);

		cuint = fdt_getprop(fdt, qspi_subnode, "inter-loopback", NULL);
		if (cuint)
			spidev.chip_config->inter_loopback = \
				fdt32_to_cpu(*cuint);
	}

	spidev.cs_timing = NULL;
	spidev.max_speed_hz = 1000 * 1000; //1Mhz
	spidev.mode = SPI_MODE_0;
	spidev.src_clk_hz = src_clk_hz;

	g_mdata.dev_comp = &ipm_compat;
	g_mdata.dev = &spidev;

	if (!g_mdata.mem_malloced) {
		SPI_TXDMA_BUF = mtk_mem_pool_alloc(SPI_TX_BUF_SIZE);
		SPI_RXDMA_BUF = mtk_mem_pool_alloc(SPI_RX_BUF_SIZE);
		g_mdata.mem_malloced = true;
	}

	mtk_spi_log("SPI_TXDMA_BUF=0x%x\n" ,SPI_TXDMA_BUF);
	mtk_spi_log("SPI_RXDMA_BUF=0x%x\n" ,SPI_RXDMA_BUF);

	return spi_mem_init_slave(fdt, qspi_node, &mtk_qspi_bus_ops);
}
