/*
 * Copyright (c) 2019, MediaTek Inc. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch_helpers.h>
#include <bl31/interrupt_mgmt.h>
#include <common/bl_common.h>
#include <common/debug.h>
#include <drivers/arm/gic_common.h>
#include <drivers/arm/gicv2.h>
#include <lib/mmio.h>
#include <plat/common/platform.h>
#include <platform_def.h>

void gicd_write_icfgr(uintptr_t base, unsigned int id, unsigned int val);

const gicv2_driver_data_t mt_gic_data = {
	.gicd_base = BASE_GICD_BASE,
	.gicc_base = BASE_GICC_BASE,
};

/* ARM common helper to initialize the GICv2 only driver. */
void plat_mt_gic_driver_init(void)
{
	gicv2_driver_init(&mt_gic_data);
}

void plat_mt_gic_init(void)
{
	int idx;

	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();

	/* set watchdog interrupt as falling edge trigger */
	gicd_write_icfgr(BASE_GICD_BASE, WDT_IRQ_BIT_ID,
			 0x2 << ((WDT_IRQ_BIT_ID % 16) * 2));

	/* set pol control as non-secure */
	for (idx = 0; idx < INT_POL_SECCTL_NUM; idx++)
		mmio_write_32(INT_POL_SECCTL0 + idx * 4, 0);
}

void irq_raise_softirq(unsigned int map, unsigned int irq)
{
	int satt = 1 << 15;

	if (plat_ic_get_interrupt_type(irq) == INTR_TYPE_S_EL1)
		satt = 0;

	mmio_write_32(BASE_GICD_BASE + GICD_SGIR, (map << 16) | satt | irq);

	dsb();
}

#ifndef MAX_GIC_NR
#define MAX_GIC_NR			(1)
#endif
#define MAX_RDIST_NR			(64)
#define DIV_ROUND_UP(n, d)		(((n) + (d) - 1) / (d))

/* For saving ATF size, we reduce 1020 -> 320 */
struct gic_chip_data {
	unsigned int saved_spi_enable[DIV_ROUND_UP(320, 32)];
	unsigned int saved_spi_conf[DIV_ROUND_UP(320, 16)];
	unsigned int saved_spi_target[DIV_ROUND_UP(320, 4)];
	unsigned int saved_spi_group[DIV_ROUND_UP(320, 32)];
};

static struct gic_chip_data gic_data[MAX_GIC_NR];

/* TODO: check all registers to save */
void mt_gic_dist_save(void)
{
	unsigned int gic_irqs;
	unsigned int dist_base;
	int i;

	/* TODO: pending bit MUST added */
	dist_base = BASE_GICD_BASE;

	gic_irqs = 32 * ((mmio_read_32(dist_base + GICD_TYPER) &
			  TYPER_IT_LINES_NO_MASK) + 1);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 16); i++)
		gic_data[0].saved_spi_conf[i] =
			mmio_read_32(dist_base + GICD_ICFGR + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 4); i++)
		gic_data[0].saved_spi_target[i] =
			mmio_read_32(dist_base + GICD_ITARGETSR + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 32); i++)
		gic_data[0].saved_spi_enable[i] =
			mmio_read_32(dist_base + GICD_ISENABLER + i * 4);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 32); i++)
		gic_data[0].saved_spi_group[i] =
			mmio_read_32(dist_base + GICD_IGROUPR + i * 4);
}

/* TODO: check all registers to restore */
void mt_gic_dist_restore(void)
{
	unsigned int gic_irqs;
	unsigned int dist_base;
	unsigned int ctlr;
	int i;

	dist_base = BASE_GICD_BASE;

	gic_irqs = 32 * ((mmio_read_32(dist_base + GICD_TYPER) &
			  TYPER_IT_LINES_NO_MASK) + 1);

	/* Disable the distributor before going further */
	ctlr = mmio_read_32(dist_base + GICD_CTLR);
	ctlr &= ~(CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT);
	mmio_write_32(dist_base + GICD_CTLR, ctlr);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 16); i++)
		mmio_write_32(dist_base + GICD_ICFGR + i * 4,
			      gic_data[0].saved_spi_conf[i]);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 4); i++)
		mmio_write_32(dist_base + GICD_ITARGETSR + i * 4,
			      gic_data[0].saved_spi_target[i]);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 32); i++)
		mmio_write_32(dist_base + GICD_ISENABLER + i * 4,
			      gic_data[0].saved_spi_enable[i]);

	for (i = 0; i < DIV_ROUND_UP(gic_irqs, 32); i++)
		mmio_write_32(dist_base + GICD_IGROUPR + i * 4,
			      gic_data[0].saved_spi_group[i]);

	mmio_write_32(dist_base + GICD_CTLR,
		      ctlr | CTLR_ENABLE_G0_BIT | CTLR_ENABLE_G1_BIT);
}

void mt_gic_cpuif_setup(void)
{
	unsigned int val;

	val = mmio_read_32(BASE_GICC_BASE + GICC_CTLR);
	val |= CTLR_ENABLE_G1_BIT;
	mmio_write_32(BASE_GICC_BASE + GICC_CTLR, val);
}
