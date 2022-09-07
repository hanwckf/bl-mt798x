/*
 * Copyright (c) 2019, MediaTek Inc.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include <stddef.h>
#include <drivers/mmc.h>
#include <stdint.h>
#include <stdbool.h>
#include <lib/mmio.h>
#include <common/debug.h>
#include <errno.h>

#include "mtk-sd.h"

/* MSDC_CFG */
#define MSDC_CFG_HS400_CK_MODE_EXT	BIT(22)
#define MSDC_CFG_CKMOD_EXT_M		0x300000
#define MSDC_CFG_CKMOD_EXT_S		20
#define MSDC_CFG_CKDIV_EXT_M		0xfff00
#define MSDC_CFG_CKDIV_EXT_S		8
#define MSDC_CFG_HS400_CK_MODE		BIT(18)
#define MSDC_CFG_CKMOD_M		0x30000
#define MSDC_CFG_CKMOD_S		16
#define MSDC_CFG_CKDIV_M		0xff00
#define MSDC_CFG_CKDIV_S		8
#define MSDC_CFG_CKSTB			BIT(7)
#define MSDC_CFG_PIO			BIT(3)
#define MSDC_CFG_RST			BIT(2)
#define MSDC_CFG_CKPDN			BIT(1)
#define MSDC_CFG_MODE			BIT(0)

/* MSDC_IOCON */
#define MSDC_IOCON_W_DSPL		BIT(8)
#define MSDC_IOCON_DSPL			BIT(2)
#define MSDC_IOCON_RSPL			BIT(1)

/* MSDC_PS */
#define MSDC_PS_DAT0			BIT(16)
#define MSDC_PS_CDDBCE_M		0xf000
#define MSDC_PS_CDDBCE_S		12
#define MSDC_PS_CDSTS			BIT(1)
#define MSDC_PS_CDEN			BIT(0)

/* #define MSDC_INT(EN) */
#define MSDC_INT_ACMDRDY		BIT(3)
#define MSDC_INT_ACMDTMO		BIT(4)
#define MSDC_INT_ACMDCRCERR		BIT(5)
#define MSDC_INT_CMDRDY			BIT(8)
#define MSDC_INT_CMDTMO			BIT(9)
#define MSDC_INT_RSPCRCERR		BIT(10)
#define MSDC_INT_XFER_COMPL		BIT(12)
#define MSDC_INT_DATTMO			BIT(14)
#define MSDC_INT_DATCRCERR		BIT(15)

/* MSDC_FIFOCS */
#define MSDC_FIFOCS_CLR			BIT(31)
#define MSDC_FIFOCS_TXCNT_M		0xff0000
#define MSDC_FIFOCS_TXCNT_S		16
#define MSDC_FIFOCS_RXCNT_M		0xff
#define MSDC_FIFOCS_RXCNT_S		0

/* #define SDC_CFG */
#define SDC_CFG_DTOC_M			0xff000000
#define SDC_CFG_DTOC_S			24
#define SDC_CFG_SDIOIDE			BIT(20)
#define SDC_CFG_SDIO			BIT(19)
#define SDC_CFG_BUSWIDTH_M		0x30000
#define SDC_CFG_BUSWIDTH_S		16

/* SDC_CMD */
#define SDC_CMD_BLK_LEN_M		0xfff0000
#define SDC_CMD_BLK_LEN_S		16
#define SDC_CMD_STOP			BIT(14)
#define SDC_CMD_WR			BIT(13)
#define SDC_CMD_DTYPE_M			0x1800
#define SDC_CMD_DTYPE_S			11
#define SDC_CMD_RSPTYP_M		0x380
#define SDC_CMD_RSPTYP_S		7
#define SDC_CMD_CMD_M			0x3f
#define SDC_CMD_CMD_S			0

/* SDC_STS */
#define SDC_STS_CMDBUSY			BIT(1)
#define SDC_STS_SDCBUSY			BIT(0)

/* SDC_ADV_CFG0 */
#define SDC_RX_ENHANCE_EN		BIT(20)

/* PATCH_BIT0 */
#define MSDC_INT_DAT_LATCH_CK_SEL_M	0x380
#define MSDC_INT_DAT_LATCH_CK_SEL_S	7

/* PATCH_BIT1 */
#define MSDC_PB1_STOP_DLY_M		0xf00
#define MSDC_PB1_STOP_DLY_S		8

/* PATCH_BIT2 */
#define MSDC_PB2_CRCSTSENSEL_M		0xe0000000
#define MSDC_PB2_CRCSTSENSEL_S		29
#define MSDC_PB2_CFGCRCSTS		BIT(28)
#define MSDC_PB2_RESPSTSENSEL_M		0x70000
#define MSDC_PB2_RESPSTSENSEL_S		16
#define MSDC_PB2_CFGRESP		BIT(15)
#define MSDC_PB2_RESPWAIT_M		0x0c
#define MSDC_PB2_RESPWAIT_S		2

/* PAD_TUNE */
#define MSDC_PAD_TUNE_CMDRRDLY_M	0x7c00000
#define MSDC_PAD_TUNE_CMDRRDLY_S	22
#define MSDC_PAD_TUNE_CMD_SEL		BIT(21)
#define MSDC_PAD_TUNE_CMDRDLY_M		0x1f0000
#define MSDC_PAD_TUNE_CMDRDLY_S		16
#define MSDC_PAD_TUNE_RXDLYSEL		BIT(15)
#define MSDC_PAD_TUNE_RD_SEL		BIT(13)
#define MSDC_PAD_TUNE_DATRRDLY_M	0x1f00
#define MSDC_PAD_TUNE_DATRRDLY_S	8
#define MSDC_PAD_TUNE_DATWRDLY_M	0x1f
#define MSDC_PAD_TUNE_DATWRDLY_S	0

/* EMMC50_CFG0 */
#define EMMC50_CFG_CFCSTS_SEL		BIT(4)

/* SDC_FIFO_CFG */
#define SDC_FIFO_CFG_WRVALIDSEL		BIT(24)
#define SDC_FIFO_CFG_RDVALIDSEL		BIT(25)

/* EMMC_TOP_CONTROL */
#define PAD_RXDLY_SEL			BIT(0)
#define DELAY_EN			BIT(1)
#define PAD_DAT_RD_RXSEL2_M		0x7c
#define PAD_DAT_RD_RXSEL2_S		2
#define PAD_DAT_RD_RXSEL_M		0xf80
#define PAD_DAT_RD_RXSEL_S		7
#define PAD_DAT_RD_RXDLY2_SEL		BIT(12)
#define PAD_DAT_RD_RXDLY_SEL		BIT(13)
#define DATA_K_VALUE_SEL		BIT(14)
#define SDC_RX_ENH_EN			BIT(15)

/* EMMC_TOP_CMD mask */
#define PAD_CMD_RXDLY2_M		0x1f
#define PAD_CMD_RXDLY2_S		0
#define PAD_CMD_RXDLY_M			0x3e0
#define PAD_CMD_RXDLY_S			5
#define PAD_CMD_RD_RXDLY2_SEL		BIT(10)
#define PAD_CMD_RD_RXDLY_SEL		BIT(11)

/* SDC_CFG_BUSWIDTH */
#define MSDC_BUS_1BITS			0x0
#define MSDC_BUS_4BITS			0x1
#define MSDC_BUS_8BITS			0x2

#define MSDC_FIFO_SIZE			128

#define PAD_DELAY_MAX			32

/* Some SD/MMC commands used by msdc_cmd_prepare_raw_cmd() */
#define MMC_CMD_SWITCH			6
#define MMC_CMD_SEND_EXT_CSD		8
#define MMC_CMD_STOP_TRANSMISSION	12
#define MMC_CMD_SEND_STATUS		13
#define MMC_CMD_READ_SINGLE_BLOCK	17
#define MMC_CMD_READ_MULTIPLE_BLOCK	18
#define MMC_CMD_WRITE_SINGLE_BLOCK	24
#define MMC_CMD_WRITE_MULTIPLE_BLOCK	25
#define SD_CMD_APP_SEND_SCR		51

/*
 * Bus clock during sd/mmc initialization should be low to ensure commands are
 * stable. After initialization completed we can increase the clock speed.
 *
 * For compatibility with SDHC and eMMC, the bus clock should not be
 * greater than 25MHz
 */
#define INIT_CLK_FREQ			1000000
#define DEFAULT_CLK_FREQ		25000000

#define CMD_INTS_MASK	\
	(MSDC_INT_CMDRDY | MSDC_INT_RSPCRCERR | MSDC_INT_CMDTMO)

#define DATA_INTS_MASK	\
	(MSDC_INT_XFER_COMPL | MSDC_INT_DATTMO | MSDC_INT_DATCRCERR)

#define readl_poll_timeout(_addr, _reg, _test, _timeout) \
			({ do { \
				_reg = mmio_read_32((uintptr_t)_addr); \
				(void)_timeout; \
			} while (!(_test)); 0; })

/* Register offset */
struct mtk_sd_regs {
	uint32_t msdc_cfg;
	uint32_t msdc_iocon;
	uint32_t msdc_ps;
	uint32_t msdc_int;
	uint32_t msdc_inten;
	uint32_t msdc_fifocs;
	uint32_t msdc_txdata;
	uint32_t msdc_rxdata;
	uint32_t reserved0[4];
	uint32_t sdc_cfg;
	uint32_t sdc_cmd;
	uint32_t sdc_arg;
	uint32_t sdc_sts;
	uint32_t sdc_resp[4];
	uint32_t sdc_blk_num;
	uint32_t sdc_vol_chg;
	uint32_t sdc_csts;
	uint32_t sdc_csts_en;
	uint32_t sdc_datcrc_sts;
	uint32_t sdc_adv_cfg0;
	uint32_t reserved1[2];
	uint32_t emmc_cfg0;
	uint32_t emmc_cfg1;
	uint32_t emmc_sts;
	uint32_t emmc_iocon;
	uint32_t sd_acmd_resp;
	uint32_t sd_acmd19_trg;
	uint32_t sd_acmd19_sts;
	uint32_t dma_sa_high4bit;
	uint32_t dma_sa;
	uint32_t dma_ca;
	uint32_t dma_ctrl;
	uint32_t dma_cfg;
	uint32_t sw_dbg_sel;
	uint32_t sw_dbg_out;
	uint32_t dma_length;
	uint32_t reserved2;
	uint32_t patch_bit0;
	uint32_t patch_bit1;
	uint32_t patch_bit2;
	uint32_t reserved3;
	uint32_t dat0_tune_crc;
	uint32_t dat1_tune_crc;
	uint32_t dat2_tune_crc;
	uint32_t dat3_tune_crc;
	uint32_t cmd_tune_crc;
	uint32_t sdio_tune_wind;
	uint32_t reserved4[5];
	uint32_t pad_tune;
	uint32_t pad_tune0;
	uint32_t pad_tune1;
	uint32_t dat_rd_dly[4];
	uint32_t reserved5[2];
	uint32_t hw_dbg_sel;
	uint32_t main_ver;
	uint32_t eco_ver;
	uint32_t reserved6[27];
	uint32_t pad_ds_tune;
	uint32_t reserved7[31];
	uint32_t emmc50_cfg0;
	uint32_t reserved8[7];
	uint32_t sdc_fifo_cfg;
};

struct msdc_tune_para {
	uint32_t iocon;
	uint32_t pad_tune;
};

struct mtk_sd_top_regs {
	uint32_t emmc_top_control;
	uint32_t emmc_top_cmd;
	uint32_t emmc50_pad_ctl0;
	uint32_t emmc50_pad_ds_tune;
};

static struct msdc_host {
	struct mtk_sd_regs *base;
	struct mtk_sd_top_regs *top_base;
	const struct msdc_compatible *dev_comp;

	uint32_t src_clk_freq;	/* source clock */
	uint32_t mclk;		/* mmc framework required bus clock */
	uint32_t sclk;		/* actual calculated bus clock */

	/* operation timeout clocks */
	uint32_t timeout_ns;
	uint32_t timeout_clks;

	unsigned int last_resp_type;
	unsigned int last_data_write;

	struct msdc_tune_para def_tune_para;
} _host;

static bool new_xfer;
static size_t xfer_size;
static uint32_t xfer_blocks;
static uint32_t xfer_blocksz;

static struct mmc_device_info mtk_mmc_device_info = {
	.mmc_dev_type = MMC_IS_SD,
	.ocr_voltage = OCR_3_2_3_3 | OCR_3_3_3_4,
};

static void msdc_reset_hw(struct msdc_host *host)
{
	uint32_t reg;

	mmio_setbits_32((uintptr_t)&host->base->msdc_cfg, MSDC_CFG_RST);

	readl_poll_timeout(&host->base->msdc_cfg, reg,
			   !(reg & MSDC_CFG_RST), 1000000);
}

static void msdc_fifo_clr(struct msdc_host *host)
{
	uint32_t reg;

	mmio_setbits_32((uintptr_t)&host->base->msdc_fifocs, MSDC_FIFOCS_CLR);

	readl_poll_timeout(&host->base->msdc_fifocs, reg,
			   !(reg & MSDC_FIFOCS_CLR), 1000000);
}

static uint32_t msdc_fifo_rx_bytes(struct msdc_host *host)
{
	return (mmio_read_32((uintptr_t)&host->base->msdc_fifocs) &
		MSDC_FIFOCS_RXCNT_M) >> MSDC_FIFOCS_RXCNT_S;
}

static uint32_t msdc_cmd_find_resp(struct msdc_host *host, struct mmc_cmd *cmd)
{
	uint32_t resp;

	switch (cmd->resp_type) {
		/* Actually, R1, R5, R6, R7 are the same */
	case MMC_RESPONSE_R1:
		resp = 0x1;
		break;
	case MMC_RESPONSE_R1B:
		resp = 0x7;
		break;
	case MMC_RESPONSE_R2:
		resp = 0x2;
		break;
	case MMC_RESPONSE_R3:
		resp = 0x3;
		break;
	default:
		resp = 0x0;
		break;
	}

	return resp;
}

static uint32_t msdc_cmd_prepare_raw_cmd(struct msdc_host *host,
					 struct mmc_cmd *cmd)
{
	uint32_t opcode = cmd->cmd_idx;
	uint32_t resp_type = msdc_cmd_find_resp(host, cmd);
	uint32_t blksz = 0;
	uint32_t dtype = 0;
	uint32_t rawcmd = 0;

	switch (opcode) {
	case MMC_CMD_WRITE_MULTIPLE_BLOCK:
	case MMC_CMD_READ_MULTIPLE_BLOCK:
		dtype = 2;
		break;
	case MMC_CMD_WRITE_SINGLE_BLOCK:
	case MMC_CMD_READ_SINGLE_BLOCK:
	case SD_CMD_APP_SEND_SCR:
		dtype = 1;
		break;
	case MMC_CMD_SWITCH:
	case MMC_CMD_SEND_EXT_CSD:
	case MMC_CMD_SEND_STATUS:
		if (new_xfer && xfer_size)
			dtype = 1;
	default:
		break;
	}

	if (new_xfer && xfer_size) {
		if (xfer_blocks > 1)
			dtype = 2;
		blksz = xfer_blocksz;
	}

	rawcmd |= ((opcode << SDC_CMD_CMD_S) & SDC_CMD_CMD_M) |
		((resp_type << SDC_CMD_RSPTYP_S) & SDC_CMD_RSPTYP_M) |
		((blksz << SDC_CMD_BLK_LEN_S) & SDC_CMD_BLK_LEN_M) |
		((dtype << SDC_CMD_DTYPE_S) & SDC_CMD_DTYPE_M);

	if (opcode == MMC_CMD_STOP_TRANSMISSION)
		rawcmd |= SDC_CMD_STOP;

	return rawcmd;
}

static int msdc_cmd_done(struct msdc_host *host, int events,
			 struct mmc_cmd *cmd)
{
	uint32_t *rsp = cmd->resp_data;
	int ret = 0;

	if (cmd->resp_type & MMC_RSP_48) {
		if (cmd->resp_type & MMC_RSP_136) {
			rsp[0] = mmio_read_32((uintptr_t)&host->base->sdc_resp[3]);
			rsp[1] = mmio_read_32((uintptr_t)&host->base->sdc_resp[2]);
			rsp[2] = mmio_read_32((uintptr_t)&host->base->sdc_resp[1]);
			rsp[3] = mmio_read_32((uintptr_t)&host->base->sdc_resp[0]);
		} else {
			rsp[0] = mmio_read_32((uintptr_t)&host->base->sdc_resp[0]);
		}
	}

	if (!(events & MSDC_INT_CMDRDY)) {
		msdc_reset_hw(host);

		if (events & MSDC_INT_CMDTMO) {
			ERROR("MSDC: Command has timed out with cmd=%d, arg=0x%x\n",
				cmd->cmd_idx, cmd->cmd_arg);
			ret = -ETIMEDOUT;
		} else if (events & MSDC_INT_RSPCRCERR){
			ERROR("MSDC: Command has CRC error with cmd=%d, arg=0x%x\n",
				cmd->cmd_idx, cmd->cmd_arg);
			ret = -EIO;
		} else {
			ERROR("MSDC: Recieve unexpected response INT with cmd=%d, arg=0x%x\n",
				cmd->cmd_idx, cmd->cmd_arg);
			ret = EINVAL;
		}
	}

	return ret;
}

static bool msdc_cmd_is_ready(struct msdc_host *host)
{
	int ret;
	uint32_t reg;

	/* The max busy time we can endure is 20ms */
	ret = readl_poll_timeout(&host->base->sdc_sts, reg,
				 !(reg & SDC_STS_CMDBUSY), 20000);

	if (ret) {
		ERROR("MSDC: CMD bus busy detected\n");
		msdc_reset_hw(host);
		return false;
	}

	if (host->last_resp_type == MMC_RESPONSE_R1B && host->last_data_write) {
		ret = readl_poll_timeout(&host->base->msdc_ps, reg,
					 reg & MSDC_PS_DAT0, 1000000);

		if (ret) {
			ERROR("MSDC: Card stuck in programming state!\n");
			msdc_reset_hw(host);
			return false;
		}
	}

	return true;
}

static int msdc_start_command(struct msdc_host *host, struct mmc_cmd *cmd)
{
	uint32_t rawcmd;
	uint32_t status;
	uint32_t blocks = 0;
	int ret;

	if (!msdc_cmd_is_ready(host))
		return -EIO;

	msdc_fifo_clr(host);

	host->last_resp_type = cmd->resp_type;
	host->last_data_write = 0;

	rawcmd = msdc_cmd_prepare_raw_cmd(host, cmd);

	if (new_xfer && xfer_size)
		blocks = xfer_blocks;

	mmio_write_32((uintptr_t)&host->base->msdc_int, CMD_INTS_MASK);
	mmio_write_32((uintptr_t)&host->base->sdc_blk_num, blocks);
	mmio_write_32((uintptr_t)&host->base->sdc_arg, cmd->cmd_arg);
	mmio_write_32((uintptr_t)&host->base->sdc_cmd, rawcmd);

	ret = readl_poll_timeout(&host->base->msdc_int, status,
				 status & CMD_INTS_MASK, 1000000);
	if (ret) {
		ERROR("MSDC: cannot wait INT with cmd=%d, arg=0x%x\n",
				cmd->cmd_idx, cmd->cmd_arg);
		status = MSDC_INT_CMDTMO;
	}

	return msdc_cmd_done(host, status, cmd);
}

static int mtk_mmc_send_cmd(struct mmc_cmd *cmd)
{
	struct msdc_host *host = &_host;

	return msdc_start_command(host, cmd);
}

static void msdc_set_buswidth(struct msdc_host *host, uint32_t width)
{
	uint32_t val = mmio_read_32((uintptr_t)&host->base->sdc_cfg);

	val &= ~SDC_CFG_BUSWIDTH_M;

	switch (width) {
	default:
	case MMC_BUS_WIDTH_1:
		val |= (MSDC_BUS_1BITS << SDC_CFG_BUSWIDTH_S);
		break;
	case MMC_BUS_WIDTH_4:
		val |= (MSDC_BUS_4BITS << SDC_CFG_BUSWIDTH_S);
		break;
	case MMC_BUS_WIDTH_8:
		val |= (MSDC_BUS_8BITS << SDC_CFG_BUSWIDTH_S);
		break;
	}

	mmio_write_32((uintptr_t)&host->base->sdc_cfg, val);
}

static void msdc_set_timeout(struct msdc_host *host, uint32_t ns, uint32_t clks)
{
	uint32_t timeout = 0, clk_ns;
	uint32_t mode = 0;

	host->timeout_ns = ns;
	host->timeout_clks = clks;

	if (host->sclk) {
		clk_ns = 1000000000UL / host->sclk;
		timeout = (ns + clk_ns - 1) / clk_ns + clks;
		/* unit is 1048576 sclk cycles */
		timeout = (timeout + (0x1 << 20) - 1) >> 20;
		if (host->dev_comp->clk_div_bits == 8)
			mode = (mmio_read_32((uintptr_t)&host->base->msdc_cfg) &
				MSDC_CFG_CKMOD_M) >> MSDC_CFG_CKMOD_S;
		else
			mode = (mmio_read_32((uintptr_t)&host->base->msdc_cfg) &
				MSDC_CFG_CKMOD_EXT_M) >> MSDC_CFG_CKMOD_EXT_S;
		/* DDR mode will double the clk cycles for data timeout */
		timeout = mode >= 2 ? timeout * 2 : timeout;
		timeout = timeout > 1 ? timeout - 1 : 0;
		timeout = timeout > 255 ? 255 : timeout;
	}

	mmio_clrsetbits_32((uintptr_t)&host->base->sdc_cfg, SDC_CFG_DTOC_M,
			   timeout << SDC_CFG_DTOC_S);
}

static void msdc_set_mclk(struct msdc_host *host, uint32_t hz)
{
	uint32_t mode;
	uint32_t div;
	uint32_t sclk;
	uint32_t reg;

	if (!hz) {
		host->mclk = 0;
		mmio_clrbits_32((uintptr_t)&host->base->msdc_cfg,
				MSDC_CFG_CKPDN);
		return;
	}

	if (host->dev_comp->clk_div_bits == 8)
		mmio_clrbits_32((uintptr_t)&host->base->msdc_cfg,
				MSDC_CFG_HS400_CK_MODE);
	else
		mmio_clrbits_32((uintptr_t)&host->base->msdc_cfg,
				MSDC_CFG_HS400_CK_MODE_EXT);

	if (hz >= host->src_clk_freq) {
		mode = 0x1; /* no divisor */
		div = 0;
		sclk = host->src_clk_freq;
	} else {
		mode = 0x0; /* use divisor */
		if (hz >= (host->src_clk_freq >> 1)) {
			div = 0; /* mean div = 1/2 */
			sclk = host->src_clk_freq >> 1; /* sclk = clk / 2 */
		} else {
			div = (host->src_clk_freq + ((hz << 2) - 1)) /
			       (hz << 2);
			sclk = (host->src_clk_freq >> 2) / div;
		}
	}

	mmio_clrbits_32((uintptr_t)&host->base->msdc_cfg, MSDC_CFG_CKPDN);

	if (host->dev_comp->clk_div_bits == 8) {
		div = MIN(div, (uint32_t)(MSDC_CFG_CKDIV_M >> MSDC_CFG_CKDIV_S));
		mmio_clrsetbits_32((uintptr_t)&host->base->msdc_cfg,
				   MSDC_CFG_CKMOD_M | MSDC_CFG_CKDIV_M,
				   (mode << MSDC_CFG_CKMOD_S) |
				   (div << MSDC_CFG_CKDIV_S));
	} else {
		div = MIN(div, (uint32_t)(MSDC_CFG_CKDIV_EXT_M >>
				     MSDC_CFG_CKDIV_EXT_S));
		mmio_clrsetbits_32((uintptr_t)&host->base->msdc_cfg,
				   MSDC_CFG_CKMOD_EXT_M | MSDC_CFG_CKDIV_EXT_M,
				   (mode << MSDC_CFG_CKMOD_EXT_S) |
				   (div << MSDC_CFG_CKDIV_EXT_S));
	}

	readl_poll_timeout(&host->base->msdc_cfg, reg,
			   reg & MSDC_CFG_CKSTB, 1000000);

	host->sclk = sclk;
	host->mclk = hz;

	/* needed because clk changed. */
	msdc_set_timeout(host, host->timeout_ns, host->timeout_clks);

	VERBOSE("MSDC: bus clock is set to %dHz\n", host->sclk);
}

static int msdc_ops_set_ios(unsigned int clock, unsigned int bus_width)
{
	struct msdc_host *host = &_host;

	msdc_set_buswidth(host, bus_width);

	if (host->mclk != clock)
		msdc_set_mclk(host, clock);

	return 0;
}

static int mtk_mmc_prepare(int lba, uintptr_t buf, size_t size)
{
	xfer_size = size;
	xfer_blocksz = mtk_mmc_device_info.block_size;

	if (!xfer_blocksz) {
		if (xfer_size < MMC_BLOCK_SIZE)
			xfer_blocksz = xfer_size;
		else
			xfer_blocksz = MMC_BLOCK_SIZE;
	}

	xfer_blocks = (size + xfer_blocksz - 1) / xfer_blocksz;
	new_xfer = true;

	return 0;
}

static void msdc_fifo_read(struct msdc_host *host, uint8_t *buf, size_t size)
{
	uint32_t *wbuf;

	while ((size_t)buf % 4) {
		*buf++ = mmio_read_8((uintptr_t)&host->base->msdc_rxdata);
		size--;
	}

	wbuf = (uint32_t *)buf;
	while (size >= 4) {
		*wbuf++ = mmio_read_32((uintptr_t)&host->base->msdc_rxdata);
		size -= 4;
	}

	buf = (uint8_t *)wbuf;
	while (size) {
		*buf++ = mmio_read_8((uintptr_t)&host->base->msdc_rxdata);
		size--;
	}
}

static int mtk_mmc_read(int lba, uintptr_t buf, size_t size)
{
	struct msdc_host *host = &_host;

	uint32_t status;
	uint32_t chksz;
	uint32_t cmd_idx, cmd_arg;
	int ret = 0;

	new_xfer = false;

	if (size > xfer_size) {
		ERROR("MSDC: Read data size exceeds prepared size\n");
		return -EINVAL;
	}

	cmd_idx = mmio_read_32((uintptr_t)&host->base->sdc_cmd) & 0x3f;
	cmd_arg = mmio_read_32((uintptr_t)&host->base->sdc_arg);

	mmio_write_32((uintptr_t)&host->base->msdc_int, DATA_INTS_MASK);

	while (1) {
		status = mmio_read_32((uintptr_t)&host->base->msdc_int);
		mmio_write_32((uintptr_t)&host->base->msdc_int, status);
		status &= DATA_INTS_MASK;

		if (status & MSDC_INT_DATCRCERR) {
			ERROR("MSDC: CRC error occured while reading data with cmd=%d, arg=0x%x\n",
				cmd_idx, cmd_arg);
			ret = -EIO;
			break;
		}

		if (status & MSDC_INT_DATTMO) {
			ERROR("MSDC: timeout occured while reading data with cmd=%d, arg=0x%x\n",
				cmd_idx, cmd_arg);
			ret = -ETIMEDOUT;
			break;
		}

		chksz = MIN(size, (size_t)MSDC_FIFO_SIZE);

		if (msdc_fifo_rx_bytes(host) >= chksz) {
			msdc_fifo_read(host, (uint8_t *)buf, chksz);
			buf += chksz;
			size -= chksz;
			xfer_size -= chksz;
		}

		if ((status & MSDC_INT_XFER_COMPL) || !size) {
			if (size) {
				ERROR("MSDC: Data is not fully read\n");
				ret = -EIO;
			}
			break;
		}
	}

	return ret;
}

static int mtk_mmc_write(int lba, uintptr_t buf, size_t size)
{
	INFO("MSDC: Write operation is not supported\n");
	return -1;
}

static void msdc_init_hw(void)
{
	uint32_t val;
	struct msdc_host *host = &_host;
	uintptr_t tune_reg = (uintptr_t)&host->base->pad_tune;

	INFO("MediaTek MMC/SD Card Controller ver %08x, eco %d\n",
	     mmio_read_32((uintptr_t)&host->base->main_ver),
	     mmio_read_32((uintptr_t)&host->base->eco_ver));

	if (host->dev_comp->pad_tune0)
		tune_reg = (uintptr_t)&host->base->pad_tune0;

	/* Configure to MMC/SD mode, clock free running */
	mmio_setbits_32((uintptr_t)&host->base->msdc_cfg, MSDC_CFG_MODE);

	/* Use PIO mode */
	mmio_setbits_32((uintptr_t)&host->base->msdc_cfg, MSDC_CFG_PIO);

	/* Reset */
	msdc_reset_hw(host);

	/* Clear all interrupts */
	val = mmio_read_32((uintptr_t)&host->base->msdc_int);
	mmio_write_32((uintptr_t)&host->base->msdc_int, val);

	/* Enable data & cmd interrupts */
	mmio_write_32((uintptr_t)&host->base->msdc_inten,
		      DATA_INTS_MASK | CMD_INTS_MASK);
	mmio_write_32(tune_reg, 0);
	if(host->top_base) {
		mmio_write_32((uintptr_t)&host->top_base->emmc_top_control, 0);
		mmio_write_32((uintptr_t)&host->top_base->emmc_top_cmd, 0);
	}
	mmio_write_32((uintptr_t)&host->base->msdc_iocon, 0);

	if (host->dev_comp->r_smpl)
		mmio_setbits_32((uintptr_t)&host->base->msdc_iocon,
				MSDC_IOCON_RSPL);
	else
		mmio_clrbits_32((uintptr_t)&host->base->msdc_iocon,
				MSDC_IOCON_RSPL);

	mmio_write_32((uintptr_t)&host->base->patch_bit0, 0x403c0046);
	mmio_write_32((uintptr_t)&host->base->patch_bit1, 0xffff4089);

	if (host->dev_comp->stop_clk_fix) {
		mmio_clrbits_32((uintptr_t)&host->base->sdc_fifo_cfg,
				SDC_FIFO_CFG_WRVALIDSEL);
		mmio_clrbits_32((uintptr_t)&host->base->sdc_fifo_cfg,
				SDC_FIFO_CFG_RDVALIDSEL);
		mmio_clrsetbits_32((uintptr_t)&host->base->patch_bit1,
				   MSDC_PB1_STOP_DLY_M,
				   3 << MSDC_PB1_STOP_DLY_S);
	}

	if (host->dev_comp->busy_check)
		mmio_clrbits_32((uintptr_t)&host->base->patch_bit1, (1 << 7));

	mmio_setbits_32((uintptr_t)&host->base->emmc50_cfg0,
			EMMC50_CFG_CFCSTS_SEL);

	if (host->dev_comp->async_fifo) {
		mmio_clrsetbits_32((uintptr_t)&host->base->patch_bit2,
				   MSDC_PB2_RESPWAIT_M,
				   3 << MSDC_PB2_RESPWAIT_S);

		if (host->dev_comp->enhance_rx) {
			if (host->top_base)
				mmio_setbits_32((uintptr_t)&host->top_base->emmc_top_control,
						SDC_RX_ENH_EN);
			else
				mmio_setbits_32((uintptr_t)&host->base->sdc_adv_cfg0,
						SDC_RX_ENHANCE_EN);
		} else {
			mmio_clrsetbits_32((uintptr_t)&host->base->patch_bit2,
					   MSDC_PB2_RESPSTSENSEL_M,
					   2 << MSDC_PB2_RESPSTSENSEL_S);
			mmio_clrsetbits_32((uintptr_t)&host->base->patch_bit2,
					   MSDC_PB2_CRCSTSENSEL_M,
					   2 << MSDC_PB2_CRCSTSENSEL_S);
		}

		/* use async fifo to avoid tune internal delay */
		mmio_clrbits_32((uintptr_t)&host->base->patch_bit2,
				MSDC_PB2_CFGRESP);
		mmio_clrbits_32((uintptr_t)&host->base->patch_bit2,
				MSDC_PB2_CFGCRCSTS);
	}

	if (host->dev_comp->data_tune) {
		if (host->top_base) {
			mmio_setbits_32((uintptr_t)&host->top_base->emmc_top_control,
					PAD_DAT_RD_RXDLY_SEL);
			mmio_clrbits_32((uintptr_t)&host->top_base->emmc_top_control,
					DATA_K_VALUE_SEL);
			mmio_setbits_32((uintptr_t)&host->top_base->emmc_top_cmd,
					PAD_CMD_RD_RXDLY_SEL);
		} else {
			mmio_setbits_32(tune_reg,
					MSDC_PAD_TUNE_RD_SEL | MSDC_PAD_TUNE_CMD_SEL);
		}
		mmio_clrsetbits_32((uintptr_t)&host->base->patch_bit0,
				   MSDC_INT_DAT_LATCH_CK_SEL_M,
				   host->dev_comp->latch_ck <<
				   MSDC_INT_DAT_LATCH_CK_SEL_S);
	} else {
		/* choose clock tune */
		mmio_setbits_32(tune_reg, MSDC_PAD_TUNE_RXDLYSEL);
	}

	/* Configure to enable SDIO mode otherwise sdio cmd5 won't work */
	mmio_setbits_32((uintptr_t)&host->base->sdc_cfg, SDC_CFG_SDIO);

	/* disable detecting SDIO device interrupt function */
	mmio_clrbits_32((uintptr_t)&host->base->sdc_cfg, SDC_CFG_SDIOIDE);

	/* Configure to default data timeout */
	mmio_clrsetbits_32((uintptr_t)&host->base->sdc_cfg, SDC_CFG_DTOC_M,
			   3 << SDC_CFG_DTOC_S);

	host->def_tune_para.iocon =
			mmio_read_32((uintptr_t)&host->base->msdc_iocon);
	host->def_tune_para.pad_tune =
			mmio_read_32((uintptr_t)&host->base->pad_tune);

	/* Set initial state: 1-bit bus width */
	msdc_ops_set_ios(INIT_CLK_FREQ, MMC_BUS_WIDTH_1);
}

static const struct mmc_ops mtk_mmc_ops = {
	.init = msdc_init_hw,
	.send_cmd = mtk_mmc_send_cmd,
	.set_ios = msdc_ops_set_ios,
	.prepare = mtk_mmc_prepare,
	.read = mtk_mmc_read,
	.write = mtk_mmc_write,
};

void mtk_mmc_init(uintptr_t reg_base,  uintptr_t top_reg_base,
		  const struct msdc_compatible *compat,
		  uint32_t src_clk, enum mmc_device_type type,
		  uint32_t bus_width)
{
	struct msdc_host *host = &_host;

	host->base = (struct mtk_sd_regs *)reg_base;
	host->top_base = (struct mtk_sd_top_regs *)top_reg_base;
	host->dev_comp = compat;

	host->src_clk_freq = src_clk;
	host->timeout_ns = 100000000;
	host->timeout_clks = 3 * 1048576;

	mtk_mmc_device_info.mmc_dev_type = type;

	mmc_init(&mtk_mmc_ops, DEFAULT_CLK_FREQ, bus_width, 0,
		 &mtk_mmc_device_info);
}
