/*----------------------------------------------------------------------------*/
#ifndef __MY1I2CH__
#define __MY1I2CH__
/*----------------------------------------------------------------------------*/
#define I2C_SDA1_GPIO 2
#define I2C_SCL1_GPIO 3
#define I2C_SDA_GPIO I2C_SDA1_GPIO
#define I2C_SCL_GPIO I2C_SCL1_GPIO
/*----------------------------------------------------------------------------*/
#define I2C_READ_STOP_DISABLE 0
#define I2C_READ_STOP_ENABLE 1
#define I2C_READ_STOP_DEFAULT I2C_READ_STOP_DISABLE
/*----------------------------------------------------------------------------*/
void     i2c_init(int32_t sda_gpio, int32_t scl_gpio);
void     i2c_set_wait_time(uint32_t wait_time);
void     i2c_set_stop_read(int32_t enable);
void     i2c_putb(uint32_t addr, uint32_t regs, uint8_t data);
uint8_t  i2c_getb(uint32_t addr, uint32_t regs);
uint32_t  i2c_puts(uint32_t addr, uint32_t regs, uint8_t* pdat, uint32_t size);
uint32_t  i2c_gets(uint32_t addr, uint32_t regs, uint8_t* pdat, uint32_t size);
uint32_t i2c_puts_raw(uint32_t addr, uint8_t* pdat, uint32_t size);
uint32_t i2c_gets_raw(uint32_t addr, uint8_t* pdat, uint32_t size);
/*----------------------------------------------------------------------------*/
#endif
/*----------------------------------------------------------------------------*/
