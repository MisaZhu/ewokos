#ifndef __EV3_I2C_H__
#define __EV3_I2C_H__

#include <stdint.h>

#define I2C_NO_STOP		0x1
#define I2C_10BIT_ADDR	0x2
#define I2C_LARGE_MEM	0x4

#define I2C_ACK			0x0
#define I2C_NAK			0x1

#define I2C_WRITE		0x0
#define I2C_READ		0x1

#define I2C_NAK_ERROR	-1

typedef struct{
	int sda;
	int scl;
	int delay;
}i2c_bus_t;

int i2c_init(i2c_bus_t *bus);

int i2c_write(i2c_bus_t *bus, uint16_t dev, uint8_t *data, int len, int flag);
int i2c_read(i2c_bus_t *bus, uint16_t dev, uint8_t *data, int len, int flag);

//24cxx eeprom interface
int i2c_mem_write(i2c_bus_t *bus, uint16_t dev, uint16_t addr, uint8_t *data, int len, int flag);
int i2c_mem_read(i2c_bus_t *bus, uint16_t dev, uint16_t addr, uint8_t *data, int len, int flag);


#endif
