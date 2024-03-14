/*
 * (C) Copyright 2015, Samsung Electronics
 * Przemyslaw Marczak <p.marczak@samsung.com>
 *
 * This file is based on: drivers/i2c/soft-i2c.c,
 * with added driver-model support and code cleanup.
 *
 * Porting to ATF by Mediatek
 * Sam Shih <sam.shih@mediatek.com> 
 */
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <drivers/gpio.h>
#include <errno.h>
#include "mt_i2c.h"
#include "i2c_gpio.h"

#define debug(ni, ...) VERBOSE(ni, ##__VA_ARGS__)

#define RETRIES	  0
#define I2C_ACK	  0
#define I2C_NOACK 1

struct i2c_gpio_bus {
	/**
	  * udelay - delay [us] between GPIO toggle operations,
	  * which is 1/4 of I2C speed clock period.
	 */
	int udelay;
	/* sda, scl */
	int sda;
	int scl;

	int (*get_sda)(struct i2c_gpio_bus *bus);
	void (*set_sda)(struct i2c_gpio_bus *bus, int bit);
	void (*set_scl)(struct i2c_gpio_bus *bus, int bit);
};

struct i2c_gpio_bus g_i2c_bus;

static int i2c_gpio_sda_get(struct i2c_gpio_bus *bus)
{
	return gpio_get_value(bus->sda);
}

static void i2c_gpio_sda_set(struct i2c_gpio_bus *bus, int bit)
{
	if (bit) {
		gpio_set_direction(bus->sda, GPIO_DIR_IN);
		//gpio_set_direction(bus->sda, GPIO_DIR_OUT);
		//gpio_set_value(bus->sda, 1);
	} else {
		gpio_set_direction(bus->sda, GPIO_DIR_OUT);
		gpio_set_value(bus->sda, 0);
	}
}

static void i2c_gpio_scl_set(struct i2c_gpio_bus *bus, int bit)
{
	int count = 0;

	if (bit) {
		gpio_set_direction(bus->scl, GPIO_DIR_IN);
		while (!gpio_get_value(bus->scl) && count++ < 100000)
			udelay(1);
		if (!gpio_get_value(bus->scl))
			debug("timeout waiting on slave to release scl\n");
	} else {
		gpio_set_direction(bus->scl, GPIO_DIR_OUT);
		gpio_set_value(bus->scl, 0);
	}
}

static void i2c_gpio_write_bit(struct i2c_gpio_bus *bus, int delay, uint8_t bit)
{
	bus->set_scl(bus, 0);
	udelay(delay);
	bus->set_sda(bus, bit);
	udelay(delay);
	bus->set_scl(bus, 1);
	udelay(2 * delay);
}

static int i2c_gpio_read_bit(struct i2c_gpio_bus *bus, int delay)
{
	int value;

	bus->set_scl(bus, 1);
	udelay(delay);
	value = bus->get_sda(bus);
	udelay(delay);
	bus->set_scl(bus, 0);
	udelay(2 * delay);

	return value;
}

/* START: High -> Low on SDA while SCL is High */
static void i2c_gpio_send_start(struct i2c_gpio_bus *bus, int delay)
{
	udelay(delay);
	bus->set_sda(bus, 1);
	udelay(delay);
	bus->set_scl(bus, 1);
	udelay(delay);
	bus->set_sda(bus, 0);
	udelay(delay);
}

/* STOP: Low -> High on SDA while SCL is High */
static void i2c_gpio_send_stop(struct i2c_gpio_bus *bus, int delay)
{
	bus->set_scl(bus, 0);
	udelay(delay);
	bus->set_sda(bus, 0);
	udelay(delay);
	bus->set_scl(bus, 1);
	udelay(delay);
	bus->set_sda(bus, 1);
	udelay(delay);
}

/* ack should be I2C_ACK or I2C_NOACK */
static void i2c_gpio_send_ack(struct i2c_gpio_bus *bus, int delay, int ack)
{
	i2c_gpio_write_bit(bus, delay, ack);
	bus->set_scl(bus, 0);
	udelay(delay);
}

/**
 * Send a reset sequence consisting of 9 clocks with the data signal high
 * to clock any confused device back into an idle state.  Also send a
 * <stop> at the end of the sequence for belts & suspenders.
 */
//static void i2c_gpio_send_reset(struct i2c_gpio_bus *bus, int delay)
//{
//	int j;
//
//	for (j = 0; j < 9; j++)
//		i2c_gpio_write_bit(bus, delay, 1);
//
//	i2c_gpio_send_stop(bus, delay);
//}

/* Set sda high with low clock, before reading slave data */
static void i2c_gpio_sda_high(struct i2c_gpio_bus *bus, int delay)
{
	bus->set_scl(bus, 0);
	udelay(delay);
	bus->set_sda(bus, 1);
	udelay(delay);
}

/* Send 8 bits and look for an acknowledgement */
static int i2c_gpio_write_byte(struct i2c_gpio_bus *bus, int delay,
			       uint8_t data)
{
	int j;
	int nack;

	for (j = 0; j < 8; j++) {
		i2c_gpio_write_bit(bus, delay, data & 0x80);
		data <<= 1;
	}

	udelay(delay);

	/* Look for an <ACK>(negative logic) and return it */
	i2c_gpio_sda_high(bus, delay);
	nack = i2c_gpio_read_bit(bus, delay);

	return nack; /* not a nack is an ack */
}

/**
 * if ack == I2C_ACK, ACK the byte so can continue reading, else
 * send I2C_NOACK to end the read.
 */
static uint8_t i2c_gpio_read_byte(struct i2c_gpio_bus *bus, int delay, int ack)
{
	int data;
	int j;

	i2c_gpio_sda_high(bus, delay);
	data = 0;
	for (j = 0; j < 8; j++) {
		data <<= 1;
		data |= i2c_gpio_read_bit(bus, delay);
	}
	i2c_gpio_send_ack(bus, delay, ack);

	return data;
}

/* send start and the slave chip address */
int i2c_send_slave_addr(struct i2c_gpio_bus *bus, int delay, uint8_t chip)
{
	i2c_gpio_send_start(bus, delay);

	if (i2c_gpio_write_byte(bus, delay, chip)) {
		i2c_gpio_send_stop(bus, delay);
		return -EIO;
	}

	return 0;
}

static int i2c_gpio_write_data(struct i2c_gpio_bus *bus, uint8_t chip,
			       uint8_t *buffer, int len,
			       bool end_with_repeated_start)
{
	uint32_t delay = bus->udelay;
	int failures = 0;

	debug("%s: chip %x buffer %p len %d\n", __func__, chip, buffer, len);
	if (i2c_send_slave_addr(bus, delay, chip << 1)) {
		debug("i2c_write, no chip responded %02X\n", chip);
		return -EIO;
	}

	while (len-- > 0) {
		if (i2c_gpio_write_byte(bus, delay, *buffer++))
			failures++;
	}

	if (!end_with_repeated_start) {
		i2c_gpio_send_stop(bus, delay);
		return failures;
	}

	if (i2c_send_slave_addr(bus, delay, (chip << 1) | 0x1)) {
		debug("i2c_write, no chip responded %02X\n", chip);
		return -EIO;
	}

	return failures;
}

static int i2c_gpio_read_data(struct i2c_gpio_bus *bus, uint8_t chip,
			      uint8_t *buffer, int len)
{
	uint32_t delay = bus->udelay;

	debug("%s: chip %x buffer: %p len %d\n", __func__, chip, buffer, len);

	while (len-- > 0)
		*buffer++ = i2c_gpio_read_byte(bus, delay, len == 0);

	i2c_gpio_send_stop(bus, delay);

	return 0;
}

static int _i2c_write_data(uint8_t chip, uint8_t *buf, int len, bool rs)
{
	struct i2c_gpio_bus *bus = &g_i2c_bus;

	return i2c_gpio_write_data(bus, chip, buf, len, rs);
}

static int _i2c_read_data(uint8_t chip, uint8_t *buf, int len)
{
	struct i2c_gpio_bus *bus = &g_i2c_bus;

	return i2c_gpio_read_data(bus, chip, buf, len);
}

const mtk_i2c_ops_t i2c_gpio_ops = {
	.i2c_write_data = _i2c_write_data,
	.i2c_read_data = _i2c_read_data,
};

int i2c_gpio_init(int sda, int scl, uint32_t speed_hz)
{
	struct i2c_gpio_bus *bus = &g_i2c_bus;
	bus->udelay = 1000000 / (speed_hz << 2);
	bus->sda = sda;
	bus->scl = scl;
	bus->set_scl = i2c_gpio_scl_set;
	bus->get_sda = i2c_gpio_sda_get;
	bus->set_sda = i2c_gpio_sda_set;

	mtk_i2c_init_ops(&i2c_gpio_ops);

	return 0;
};

int mtk_i2c_init(void)
{
	VERBOSE("I2C: i2c_gpio detected: sda=gpio%d scl=gpio%d speed=%dhz\n",
	       I2C_GPIO_SDA_PIN, I2C_GPIO_SCL_PIN, I2C_DEFAULT_SPEED);
	i2c_gpio_init(I2C_GPIO_SDA_PIN, I2C_GPIO_SCL_PIN, I2C_DEFAULT_SPEED);

	return 0;
}
