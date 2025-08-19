#include <assert.h>
#include <common/fdt_wrappers.h>
#include <common/debug.h>
#include <drivers/delay_timer.h>
#include <errno.h>
#include <libfdt.h>
#include <lib/mmio.h>
#include <platform_def.h>
#include "mt_i2c.h"

static const mtk_i2c_ops_t *ops;

__attribute__((weak)) int mtk_i2c_init(void)
{
	ERROR("I2C: No avaliable i2c driver detected\n");

	return 0;
}

int mtk_i2c_init_ops(const mtk_i2c_ops_t *ops_ptr)
{
	assert(ops_ptr != 0 && (ops_ptr->i2c_read_data != 0) &&
	       (ops_ptr->i2c_write_data != 0));

	ops = ops_ptr;

	return 0;
}

int mtk_i2c_read_data(uint8_t chip, uint8_t *buf, int len)
{
	assert(ops);
	assert(ops->i2c_read_data != 0);

	return ops->i2c_read_data(chip, buf, len);
}

int mtk_i2c_write_data(uint8_t chip, uint8_t *buf, int len, bool rs)
{
	assert(ops);
	assert(ops->i2c_write_data != 0);

	return ops->i2c_write_data(chip, buf, len, rs);
}

int mtk_i2c_read_reg(uint8_t chip, uint32_t addr, int alen, uint8_t *data)
{
	uint8_t addr_buffer[2] = { 0x0, 0x0 };
	uint8_t data_buffer[2] = { 0x0, 0x0 };
	uint8_t *tmp_addr = addr_buffer;
	uint8_t *tmp_data = data_buffer;
	int ret;

	if (alen == 1) {
		*(tmp_addr + 0) = (uint8_t)(addr & 0xff);
		mtk_i2c_write_data(chip, tmp_addr, 1, 1);
	} else if (alen == 2) {
		*(tmp_addr + 0) = (uint8_t)((addr & 0xff00) >> 8);
		*(tmp_addr + 1) = (uint8_t)(addr & 0xff);
		mtk_i2c_write_data(chip, tmp_addr, 2, 1);
	} else {
		return -EINVAL;
	}

	ret = mtk_i2c_read_data(chip, tmp_data, 1);

	if (ret)
		return ret;

	*data = *(tmp_data + 0);

	return 0;
}

int mtk_i2c_write_reg(uint8_t chip, uint32_t addr, int alen, uint8_t data)
{
	uint8_t write_buffer[3] = { 0x0, 0x0, 0x0 };
	uint8_t *tmp_write = write_buffer;
	int ret;

	if (alen == 1) {
		*(tmp_write + 0) = (uint8_t)(addr & 0xff);
		*(tmp_write + 1) = data;
		ret = mtk_i2c_write_data(chip, tmp_write, 2, 0);
	} else if (alen == 2) {
		*(tmp_write + 0) = (uint8_t)((addr & 0xff00) >> 8);
		*(tmp_write + 1) = (uint8_t)(addr & 0xff);
		*(tmp_write + 2) = data;
		ret = mtk_i2c_write_data(chip, tmp_write, 3, 0);
	} else {
		return -EINVAL;
	}

	if (ret)
		return ret;

	return 0;
}
