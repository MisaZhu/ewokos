
#include "../include/arch/ev3/i2c.h" 
#include <bsp/bsp_gpio.h>

static inline void i2c_delay(i2c_bus_t *bus){
	volatile int delay = bus->delay;
	while(delay--){
	}
}

static inline void i2c_start(i2c_bus_t *bus){
	gpio_set(bus->sda, 1);
	gpio_set(bus->scl, 1);

	i2c_delay(bus);
	gpio_set(bus->sda, 0);
	i2c_delay(bus);
	gpio_set(bus->scl, 0);
	i2c_delay(bus);
}

static inline void i2c_stop(i2c_bus_t *bus){
	gpio_set(bus->sda, 0);
	gpio_set(bus->scl, 0);

	i2c_delay(bus);
	gpio_set(bus->scl, 1);
	i2c_delay(bus);
	gpio_set(bus->sda, 1);
}

static inline int i2c_trans_byte(i2c_bus_t* bus, uint8_t byte){
	uint8_t mask = 0x80;
	for(int i = 0; i < 8; i++){
		gpio_set(bus->scl, 0);
		if(byte | mask)
			gpio_set(bus->sda, 1);
		else
			gpio_set(bus->sda, 0);
		i2c_delay(bus);
		gpio_set(bus->scl, 1);
		i2c_delay(bus);
	}
	gpio_config(bus->sda, GPIO_INPUT);
	gpio_set(bus->scl, 0);
	i2c_delay(bus);
	int ret = gpio_get(bus->sda);
	gpio_set(bus->scl, 1);
	gpio_config(bus->sda, GPIO_OUTPUT);

	return ret;
}

static inline uint8_t i2c_recive_type(i2c_bus_t* bus, int ack){
	uint8_t ret = 0;
	gpio_config(bus->sda, GPIO_INPUT);
	for(int i = 0; i < 8; i++){
		ret <<= 1;
		gpio_set(bus->scl, 0);
		i2c_delay(bus);
		gpio_set(bus->scl, 1);
		if(gpio_get(bus->scl))
			ret |= 0x1;
		i2c_delay(bus);
	}
	gpio_config(bus->sda, GPIO_OUTPUT);
	gpio_set(bus->scl, 0);
	if(ack == I2C_ACK)
		gpio_set(bus->sda, 0);
	i2c_delay(bus);
	gpio_set(bus->scl, 1);
	i2c_delay(bus);
	return ret;
}

static inline int i2c_select_dev(i2c_bus_t* bus, uint16_t dev, uint8_t rw, int flag){
	if(flag & I2C_10BIT_ADDR){
		if(i2c_trans_byte(bus, 0xF0 | ((dev >> 8)&0x3) << 1 | rw) == I2C_NAK)
			return I2C_NAK;
		return i2c_trans_byte(bus, dev & 0xFF);
	}else{
		return i2c_trans_byte(bus, (dev << 1) | rw);
	}
}

int i2c_init(i2c_bus_t *bus){
	if(!bus)
		return -1;
		
	gpio_config(bus->sda, GPIO_OUTPUT);
	gpio_config(bus->scl, GPIO_OUTPUT);

	return 0;
}

int i2c_write(i2c_bus_t *bus, uint16_t dev, uint8_t *data, int len, int flag){
	i2c_start(bus);
	if(i2c_select_dev(bus, dev, I2C_WRITE, flag) == I2C_NAK)
		return I2C_NAK_ERROR;

	for(int  i = 0; i < len; i++){
		if(i2c_trans_byte(bus, data[i]) == I2C_NAK)	
			return I2C_NAK_ERROR;
	}

	if(!(flag&I2C_NO_STOP))
		i2c_stop(bus);

	return len;
}

int i2c_read(i2c_bus_t *bus, uint16_t dev, uint8_t *data, int len, int flag){
	i2c_start(bus);
	if(i2c_select_dev(bus, dev, I2C_READ, flag) == I2C_NAK)
		return I2C_NAK_ERROR;

	for(int  i = 0; i < len - 1; i++){
		data[i] = i2c_recv_byte(bus, I2C_ACK);	
	}

	data[len - 1] = i2c_recv_byte(bus, I2C_NAK);	
	return len;
}

//24cxx eeprom interface
int i2c_mem_write(i2c_bus_t *bus, uint16_t dev, uint16_t addr, uint8_t *data, int len, int flag){
	i2c_start(bus);
	if(i2c_select_dev(bus, dev, I2C_WRITE, flag) == I2C_NAK)
        return I2C_NAK_ERROR;

	if(flag & I2C_LARGE_MEM){
		if(i2c_trans_byte(bus, dev>>8) == I2C_NAK)
			 return I2C_NAK_ERROR;
	}

	if(i2c_trans_byte(bus, dev&0xFF) == I2C_NAK)
		return I2C_NAK_ERROR;

	for(int  i = 0; i < len; i++){
        if(i2c_trans_byte(bus, data[i]) == I2C_NAK)
			return I2C_NAK_ERROR;
	}
    i2c_stop(bus);

	return len;
}

int i2c_mem_read(i2c_bus_t *bus, uint16_t dev, uint16_t addr, uint8_t *data, int len, int flag){
	i2c_start(bus);
	if(i2c_select_dev(bus, dev, I2C_WRITE, flag) == I2C_NAK)
        return I2C_NAK_ERROR;

	if(flag & I2C_LARGE_MEM){
		if(i2c_trans_byte(bus, addr>>8) == I2C_NAK)
			 return I2C_NAK_ERROR;
	}

	if(i2c_trans_byte(bus, addr&0xFF) == I2C_NAK)
		return I2C_NAK_ERROR;

	return i2c_read(bus, dev, data, len, 0);
}


