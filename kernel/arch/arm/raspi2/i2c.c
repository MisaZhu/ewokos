#include "dev/i2c.h"
#include "dev/gpio.h"
#include "gpio_arch.h"
#include "kernel/system.h"
#include "dev/timer.h"

static uint32_t i2c_sda, i2c_scl;
bool i2c_stop;
static uint32_t i2c_wait, i2c_free, i2c_last;

/** routine i2c to write out start marker */
static void i2c_do_start(void) {
	/* enforce bus free time */
	while ((timer_read_sys_usec()-i2c_last) < i2c_free);
	/* now we are driving! master! */
	gpio_config(i2c_scl, GPIO_OUTPUT);
	gpio_config(i2c_sda, GPIO_OUTPUT);
	gpio_write(i2c_sda, 1);
	_delay_usec(i2c_wait);
	gpio_write(i2c_scl, 1);
	_delay_usec(i2c_wait); /* start condition setup time */
	gpio_write(i2c_sda, 0);
	_delay_usec(i2c_wait); /* start condition hold time */
	gpio_write(i2c_scl, 0);
}

/** routine i2c to write out stop marker */
static void i2c_do_stop(void) {
	gpio_write(i2c_sda, 0);
	_delay_usec(i2c_wait);
	gpio_write(i2c_scl, 1);
	/* should check clock stretching? in case slave pull scl low! */
	_delay_usec(i2c_wait); /* stop condition setup time */
	gpio_write(i2c_sda, 1);
	/* no hold time for stop condition? :p */
	_delay_usec(i2c_wait);
	/* stop driving! release lines */
	gpio_config(i2c_sda, GPIO_INPUT);
	gpio_config(i2c_scl, GPIO_INPUT);
	/* to check bus free time */
	i2c_last = timer_read_sys_usec();
}

/** routine i2c to write 1 bit */
static void i2c_do_write_bit(uint8_t data) {
	gpio_write(i2c_sda, data);
	_delay_usec(i2c_wait);
	gpio_write(i2c_scl, 1);
	_delay_usec(i2c_wait);
	/* should check clock stretching? in case slave pull scl low! */
	gpio_write(i2c_scl, 0);
}

/** routine i2c to read 1 bit */
static uint8_t i2c_do_read_bit(void) {
	uint8_t data = 0;
	/* release the line for slave */
	gpio_write(i2c_sda, 1);
	gpio_config(i2c_sda, GPIO_INPUT);
	_delay_usec(i2c_wait);
	gpio_write(i2c_scl, 1);
	/* should check clock stretching? in case slave pull scl low! */
	_delay_usec(i2c_wait);
	if (gpio_read(i2c_sda) != 0)
		data = 1;
	gpio_write(i2c_scl, 0);
	/* retake the line */
	gpio_config(i2c_sda, GPIO_OUTPUT);
	return data;
}

/** routine i2c to write 1 byte out */
static int32_t i2c_do_write_byte(uint8_t data) {
	int32_t loop, mask = 0x80;
	for (loop=0; loop<8; loop++) {
		i2c_do_write_bit(data&mask);
		mask >>= 1;
	}
	return i2c_do_read_bit();
}

/** routine i2c to read 1 byte in (acknowledge optional) */
static int32_t i2c_do_read_byte(uint8_t ack) {
	int32_t loop, mask = 0x80, data = 0x0;
	for (loop=0; loop<8; loop++) {
		if (i2c_do_read_bit() != 0)
			data |= mask;
		mask >>= 1;
	}
	i2c_do_write_bit(ack!=0 ? 0:1);
	return data;
}

void i2c_init(uint32_t sda_gpio, uint32_t scl_gpio) {
	/* assign gpio */
	i2c_sda = sda_gpio;
	i2c_scl = scl_gpio;
	/* set pull-up on pins */
	gpio_pull(i2c_sda, GPIO_PULL_UP);
	gpio_pull(i2c_scl, GPIO_PULL_UP);
	/* setup i2c sda1/scl1 as input (tri-state) - in case multiple masters */
	gpio_config(i2c_sda, GPIO_INPUT);
	gpio_config(i2c_scl, GPIO_INPUT);
	/* just prepare this */
	gpio_write(i2c_sda, 1);
	gpio_write(i2c_scl, 1);
	/* default parameters */
	i2c_wait = I2C_WAIT_DEFAULT;
	i2c_free = I2C_WAIT_DEFAULT;
	i2c_stop = 0;
	/* to check bus free time */
	i2c_last = timer_read_sys_usec();
}

void i2c_set_wait_time(uint32_t wait_time) {
	i2c_wait = wait_time;
}

void i2c_set_free_time(uint32_t free_time) {
	i2c_free = free_time;
}

void i2c_set_stop_read(bool enable) {
	i2c_stop = enable ? 1 : 0;
}

void i2c_putb(uint32_t addr, uint32_t regs, uint8_t data) {
	addr <<= 1;
	i2c_do_start();
	i2c_do_write_byte(addr);
	i2c_do_write_byte(regs);
	i2c_do_write_byte(data);
	i2c_do_stop();
}

uint8_t i2c_getb(uint32_t addr, uint32_t regs) {
	uint8_t data;
	addr <<= 1;
	i2c_do_start();
	i2c_do_write_byte(addr);
	i2c_do_write_byte(regs);
	if (i2c_stop) 
		i2c_do_stop();
	i2c_do_start();
	i2c_do_write_byte(addr|0x01); /* activate read bit */
	data = i2c_do_read_byte(0);
	i2c_do_stop();
	return data & 0xff;
}

uint32_t i2c_puts(uint32_t addr, uint32_t regs, uint8_t* pdat, uint32_t size) {
	uint32_t loop, test = 0;
	addr <<= 1;
	i2c_do_start();
	test |= i2c_do_write_byte(addr);
	test |= i2c_do_write_byte(regs);
	for (loop=0; loop<size; loop++)
		test |= i2c_do_write_byte(pdat[loop]);
	i2c_do_stop();
	return test;
}

uint32_t i2c_gets(uint32_t addr, uint32_t regs, uint8_t* pdat, uint32_t size) {
	uint32_t loop, test = 0;
	addr <<= 1;
	i2c_do_start();
	test |= i2c_do_write_byte(addr);
	test |= i2c_do_write_byte(regs);
	if (i2c_stop)
		i2c_do_stop();
	i2c_do_start();
	test |= i2c_do_write_byte(addr|0x01); /* activate read bit */
	for (loop=0; loop<size-1; loop++)
		pdat[loop] = i2c_do_read_byte(1);
	pdat[loop] = i2c_do_read_byte(0);
	i2c_do_stop();
	return test;
}
