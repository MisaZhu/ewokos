/*----------------------------------------------------------------------------*/
/**
 * - for accessing i2c
 *   = using bit-banging technique
**/
/*----------------------------------------------------------------------------*/
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/i2c.h>
#include <unistd.h>

/*----------------------------------------------------------------------------*/
static int32_t i2c_sda, i2c_scl, i2c_stop;
static uint32_t i2c_wait;
/*----------------------------------------------------------------------------*/
/** routine i2c to write out start marker */
void i2c_do_start(void) {
	/* enforce bus free time */
	usleep(i2c_wait);
	/* now we are driving! master! */
	bcm283x_gpio_config(i2c_scl,GPIO_OUTPUT);
	bcm283x_gpio_config(i2c_sda,GPIO_OUTPUT);
	bcm283x_gpio_set(i2c_sda);
	usleep(i2c_wait);
	bcm283x_gpio_set(i2c_scl);
	usleep(i2c_wait); /* start condition setup time */
	bcm283x_gpio_clr(i2c_sda);
	usleep(i2c_wait); /* start condition hold time */
	bcm283x_gpio_clr(i2c_scl);
}
/*----------------------------------------------------------------------------*/
/** routine i2c to write out stop marker */
void i2c_do_stop(void) {
	bcm283x_gpio_clr(i2c_sda);
	usleep(i2c_wait);
	bcm283x_gpio_set(i2c_scl);
	/* should check clock stretching? in case slave pull scl low! */
	usleep(i2c_wait); /* stop condition setup time */
	bcm283x_gpio_set(i2c_sda);
	/* no hold time for stop condition? :p */
	usleep(i2c_wait);
	/* stop driving! release lines */
	bcm283x_gpio_config(i2c_sda,GPIO_INPUT);
	bcm283x_gpio_config(i2c_scl,GPIO_INPUT);
}
/*----------------------------------------------------------------------------*/
/** routine i2c to write 1 bit */
void i2c_do_write_bit(int32_t data) {
	if (data) bcm283x_gpio_set(i2c_sda);
	else bcm283x_gpio_clr(i2c_sda);
	usleep(i2c_wait);
	bcm283x_gpio_set(i2c_scl);
	usleep(i2c_wait);
	/* should check clock stretching? in case slave pull scl low! */
	bcm283x_gpio_clr(i2c_scl);
}
/*----------------------------------------------------------------------------*/
/** routine i2c to read 1 bit */
int32_t i2c_do_read_bit(void) {
	int32_t data = 0;
	/* release the line for slave */
	bcm283x_gpio_set(i2c_sda);
	bcm283x_gpio_config(i2c_sda,GPIO_INPUT);
	usleep(i2c_wait);
	bcm283x_gpio_set(i2c_scl);
	/* should check clock stretching? in case slave pull scl low! */
	usleep(i2c_wait);
	if (bcm283x_gpio_read(i2c_sda)) data = 1;
	bcm283x_gpio_clr(i2c_scl);
	/* retake the line */
	bcm283x_gpio_config(i2c_sda,GPIO_OUTPUT);
	return data;
}
/*----------------------------------------------------------------------------*/
/** routine i2c to write 1 byte out */
int32_t i2c_do_write_byte(int32_t data) {
	int32_t loop, mask = 0x80;
	for (loop=0;loop<8;loop++) {
		i2c_do_write_bit(data&mask);
		mask >>= 1;
	}
	return i2c_do_read_bit();
}
/*----------------------------------------------------------------------------*/
/** routine i2c to read 1 byte in (acknowledge optional) */
int32_t i2c_do_read_byte(int32_t ack) {
	int32_t loop, mask = 0x80, data = 0x0;
	for (loop=0;loop<8;loop++) {
		if (i2c_do_read_bit())
			data |= mask;
		mask >>= 1;
	}
	i2c_do_write_bit(ack?0:1);
	return data;
}
/*----------------------------------------------------------------------------*/
void i2c_init(int32_t sda_gpio, int32_t scl_gpio) {
	_mmio_base = mmio_map(false);
	/* assign gpio */
	i2c_sda = sda_gpio;
	i2c_scl = scl_gpio;
	/* set pull-up on pins */
	bcm283x_gpio_pull(i2c_sda,GPIO_PULL_UP);
	bcm283x_gpio_pull(i2c_scl,GPIO_PULL_UP);
	/* setup i2c sda1/scl1 as input (tri-state) - in case multiple masters */
	bcm283x_gpio_config(i2c_sda,GPIO_INPUT);
	bcm283x_gpio_config(i2c_scl,GPIO_INPUT);
	/* just prepare this */
	bcm283x_gpio_set(i2c_sda);
	bcm283x_gpio_set(i2c_scl);
	/* default parameters */
	i2c_wait = 1000; /*1 msec*/
	i2c_stop = 0;
}
/*----------------------------------------------------------------------------*/
void i2c_set_wait_time(uint32_t wait_time) {
	i2c_wait = wait_time;
}
/*----------------------------------------------------------------------------*/
void i2c_set_stop_read(int32_t enable) {
	i2c_stop = enable ? 1 : 0;
}
/*----------------------------------------------------------------------------*/
void i2c_putb(int32_t addr, int32_t regs, int32_t data) {
	addr <<= 1;
	i2c_do_start();
	i2c_do_write_byte(addr);
	i2c_do_write_byte(regs);
	i2c_do_write_byte(data);
	i2c_do_stop();
}
/*----------------------------------------------------------------------------*/
int32_t i2c_getb(int32_t addr, int32_t regs) {
	int32_t data;
	addr <<= 1;
	i2c_do_start();
	i2c_do_write_byte(addr);
	i2c_do_write_byte(regs);
	if (i2c_stop) i2c_do_stop();
	i2c_do_start();
	i2c_do_write_byte(addr|0x01); /* activate read bit */
	data = i2c_do_read_byte(0);
	i2c_do_stop();
	return data & 0xff;
}
/*----------------------------------------------------------------------------*/
int32_t i2c_puts(int32_t addr, int32_t regs, int32_t* pdat, int32_t size) {
	int32_t loop, test = 0;
	addr <<= 1;
	i2c_do_start();
	test |= i2c_do_write_byte(addr);
	test |= i2c_do_write_byte(regs);
	for (loop=0;loop<size;loop++)
		test |= i2c_do_write_byte(pdat[loop]);
	i2c_do_stop();
	return test;
}
/*----------------------------------------------------------------------------*/
int32_t i2c_gets(int32_t addr, int32_t regs, int32_t* pdat, int32_t size) {
	int32_t loop, test = 0;
	addr <<= 1;
	i2c_do_start();
	test |= i2c_do_write_byte(addr);
	test |= i2c_do_write_byte(regs);
	if (i2c_stop) i2c_do_stop();
	i2c_do_start();
	test |= i2c_do_write_byte(addr|0x01); /* activate read bit */
	for (loop=0;loop<size-1;loop++)
		pdat[loop] = i2c_do_read_byte(1);
	pdat[loop] = i2c_do_read_byte(0);
	i2c_do_stop();
	return test;
}
/*----------------------------------------------------------------------------*/
