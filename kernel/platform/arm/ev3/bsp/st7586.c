#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <kernel/system.h>


/* controller-specific commands */
#define ST7586_DISP_MODE_GRAY   0x38
#define ST7586_DISP_MODE_MONO   0x39
#define ST7586_ENABLE_DDRAM 0x3a
#define ST7586_SET_DISP_DUTY    0xb0
#define ST7586_SET_PART_DISP    0xb4
#define ST7586_SET_NLINE_INV    0xb5
#define ST7586_SET_VOP      0xc0
#define ST7586_SET_BIAS_SYSTEM  0xc3
#define ST7586_SET_BOOST_LEVEL  0xc4
#define ST7586_SET_VOP_OFFSET   0xc7
#define ST7586_ENABLE_ANALOG    0xd0
#define ST7586_AUTO_READ_CTRL   0xd7
#define ST7586_OTP_RW_CTRL  0xe0
#define ST7586_OTP_CTRL_OUT 0xe1
#define ST7586_OTP_READ     0xe3

#define BIT(x)	((0x1) << x)
#define ST7586_DISP_CTRL_MX BIT(6)
#define ST7586_DISP_CTRL_MY BIT(7)

#include "spi.h"
#include "gpio.h"

#define RESET_PIN	(80)
#define CMD_PIN		(43)
#define CS_PIN		(44)


static void msleep(int ms){
	_delay_usec(ms * 1000);
}

static void st7586_command0(uint8_t cmd){
	gpio_set(CS_PIN, 0);
	gpio_set(CMD_PIN, 0);
	_delay_usec(10);
	davinci_spi_write(1, &cmd, SPI_XFER_END);
	_delay_usec(10);
	gpio_set(CMD_PIN, 1);
	gpio_set(CS_PIN, 1);
}


static void st7586_command1(uint8_t cmd, uint8_t data0){
	gpio_set(CMD_PIN, 0);
	gpio_set(CS_PIN, 0);
	_delay_usec(10);
	davinci_spi_write(1, &cmd, 0);
	gpio_set(CMD_PIN, 1);
	_delay_usec(10);
	davinci_spi_write(1, &data0, SPI_XFER_END);
	_delay_usec(10);
	gpio_set(CS_PIN, 1);
}

static void st7586_command2(uint8_t cmd, uint8_t data0, uint8_t data1){
	uint8_t data[2] = {data0, data1};
	gpio_set(CMD_PIN, 0);
	gpio_set(CS_PIN, 0);
	_delay_usec(10);
	davinci_spi_write(1, &cmd, 0);
	_delay_usec(10);
	gpio_set(CMD_PIN, 1);
	_delay_usec(10);
	davinci_spi_write(2, data, SPI_XFER_END);
	_delay_usec(10);
	gpio_set(CS_PIN, 1);
}

static void st7586_command4(uint8_t cmd, uint8_t data0, uint8_t data1, uint8_t data2, uint8_t data3){
	uint8_t data[4] = {data0, data1, data2, data3};
	gpio_set(CMD_PIN, 0);
	gpio_set(CS_PIN, 0);
	_delay_usec(10);
	davinci_spi_write(1, &cmd, 0);
	_delay_usec(10);
	gpio_set(CMD_PIN, 1);
	_delay_usec(10);
	davinci_spi_write(4, data, SPI_XFER_END);
	_delay_usec(10);
	gpio_set(CS_PIN, 1);
}

static const uint8_t st7586_lookup[] = { 0x7, 0x4, 0x2, 0x0 };

void st7586_update(uint8_t* buf, int width, int height){
	uint8_t cmd = 0x2C;
	uint8_t val;
	int w = ((width) / 3);
	st7586_command4(0x2B, 0, 0, height>>8, height&0xFF);
    st7586_command4(0x2A, 0, 0, w>>8, w&0xFF);

	gpio_set(CS_PIN, 0);
	gpio_set(CMD_PIN, 0);
	_delay_usec(10);
	davinci_spi_write(1, &cmd, 0);
	_delay_usec(10);
	gpio_set(CMD_PIN, 1);
	_delay_usec(10);
	for(int i = 0; i < height; i++){
		uint8_t *p = &buf[i*width];
		for(int j = 0; j < width; j+=3){
			val = st7586_lookup[*p++ >> 6] << 5;
			val |= st7586_lookup[*p++ >> 6] << 2;
			val |= st7586_lookup[*p++ >> 6] >> 1;
			davinci_spi_write(1, &val, 0);
		}
	}
	_delay_usec(10);
	gpio_set(CS_PIN, 1);
}

void st7586_init(void)
{
	gpio_init();

//	gpio_direction(RESET_PIN, 1, 0);
	gpio_direction(CMD_PIN, 1, 1);
	gpio_direction(CS_PIN, 1, 1);

	msleep(20);
	gpio_set(RESET_PIN, 1);
	gpio_set(CMD_PIN, 1);
	msleep(20);

	davinci_spi_claim_bus(0);

    st7586_command1(ST7586_AUTO_READ_CTRL, 0x9f);
    st7586_command1(ST7586_OTP_RW_CTRL, 0x00);

    msleep(10);

    st7586_command0(ST7586_OTP_READ);

    msleep(20);

    st7586_command0(ST7586_OTP_CTRL_OUT);

    msleep(50);

    st7586_command1(ST7586_SET_VOP_OFFSET, 0x00);
    st7586_command2(ST7586_SET_VOP, 0xe3, 0x00);
    st7586_command1(ST7586_SET_BIAS_SYSTEM, 0x02);
    st7586_command1(ST7586_SET_BOOST_LEVEL, 0x04);
    st7586_command1(ST7586_ENABLE_ANALOG, 0x1d);
    st7586_command1(ST7586_SET_NLINE_INV, 0x00);
    st7586_command0(ST7586_DISP_MODE_GRAY);
	st7586_command1(ST7586_ENABLE_DDRAM, 0x02);

    st7586_command1(ST7586_SET_DISP_DUTY, 0x7f);
    st7586_command1(ST7586_SET_PART_DISP, 0xa0);
}
