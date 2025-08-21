/* SPDX-License-Identifier: BSD-3-Clause */
/*
 * Copyright (c) 2024, MediaTek Inc. All rights reserved.
 *
 * Author: Sam Shih <sam.shih@mediatek.com>
 */

#include <assert.h>
#include <common/debug.h>
#include "mt_i2c.h"
#include "at24c.h"

int at24c_read(uint32_t offset, uint8_t *data)
{
	int ret;

	if (offset >= AT24C_DEMO_EEPROM_DATA_SIZE ||
	    offset >= (1 << (AT24C_DEMO_EEPROM_ADDR_SIZE * 8)))
		return -1;

	ret = mtk_i2c_read_reg(AT24C_DEMO_EEPROM_I2C_SLAVE, offset,
			       AT24C_DEMO_EEPROM_ADDR_SIZE, data);

	return ret;
}
