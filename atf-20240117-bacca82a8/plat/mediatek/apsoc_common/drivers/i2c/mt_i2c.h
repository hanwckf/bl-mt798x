#ifndef I2C_H
#define I2C_H

#include <stdint.h>

typedef struct mtk_i2c_ops {
	int (*i2c_write_data)(uint8_t chip, uint8_t *buf, int len, bool rs);
	int (*i2c_read_data)(uint8_t chip, uint8_t *buf, int len);
} mtk_i2c_ops_t;

int mtk_i2c_init(void);
int mtk_i2c_init_ops(const mtk_i2c_ops_t *ops_ptr);
int mtk_i2c_write_data(uint8_t chip, uint8_t *buf, int len, bool rs);
int mtk_i2c_read_data(uint8_t chip, uint8_t *buf, int len);
int mtk_i2c_write_reg(uint8_t chip, uint32_t addr, int alen, uint8_t data);
int mtk_i2c_read_reg(uint8_t chip, uint32_t addr, int alen, uint8_t *data);

#endif // I2C_H
