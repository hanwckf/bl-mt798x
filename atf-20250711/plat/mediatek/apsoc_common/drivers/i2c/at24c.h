#ifndef AT24C_DEMO_H
#define AT24C_DEMO_H

#include <stdint.h>

#define AT24C_DEMO_EEPROM_I2C_SLAVE	0x50
#define AT24C_DEMO_EEPROM_ADDR_SIZE	0x2
#define AT24C_DEMO_EEPROM_DATA_SIZE	4096

//---------------------- EXPORT API ---------------------------
int at24c_read(uint32_t offset, uint8_t *data);

#endif /* AT24C32_DEMO_H */
