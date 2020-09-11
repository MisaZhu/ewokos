#include "arch/arm/bcm283x/gpio.h"
#include "arch/arm/bcm283x/spi.h"
#include <unistd.h>

#define EPD_RST_PIN      17
#define EPD_DC_PIN       25
#define EPD_CS_PIN       8
#define EPD_BUSY_PIN     24

#define EPD_WIDTH       104
#define EPD_HEIGHT      212

static inline void wait_msec(uint32_t n) {
	usleep(n*1000);
}

//bc
static void epaper_reset(void) {
	gpio_write(EPD_RST_PIN, 1);
	wait_msec(200);
	gpio_write(EPD_RST_PIN, 0);
	wait_msec(200);
	gpio_write(EPD_RST_PIN, 1);
	wait_msec(200);
}

static void epaper_cmd_raw(uint8_t reg) {
	gpio_write(EPD_DC_PIN, 0);
	gpio_write(EPD_CS_PIN, 0);
	spi_transfer(reg);
	gpio_write(EPD_CS_PIN, 1);
}

static void epaper_cmd(uint8_t reg) {
	spi_activate(1);
	epaper_cmd_raw(reg);
	spi_activate(0);
}

static inline void epaper_write_raw(uint8_t data) {
	gpio_write(EPD_DC_PIN, 1);
	gpio_write(EPD_CS_PIN, 0);
	spi_transfer(data);
	gpio_write(EPD_CS_PIN, 1);
}

static void epaper_write(uint8_t data) {
	spi_activate(1);
	epaper_write_raw(data);
	spi_activate(0);
}

static void epaper_wait(void) {
	while(gpio_read(EPD_BUSY_PIN) == 0) {
		wait_msec(5);
	}
}

static void epaper_on(void) {
	epaper_cmd(0x12);		 //DISPLAY REFRESH
	epaper_wait();
}

static void epaper_off(void) {
	epaper_cmd(0x02); // POWER_OFF
	epaper_wait();
	epaper_cmd(0x07); // DEEP_SLEEP
	epaper_write(0xA5); // check code
}

static void epaper_init(void) {
	epaper_reset();

	epaper_cmd(0x06); // BOOSTER_SOFT_START
	epaper_write(0x17);
	epaper_write(0x17);
	epaper_write(0x17);

	epaper_cmd(0x04); // POWER_ON
	epaper_wait();

	epaper_cmd(0x00); // PANEL_SETTING
	epaper_write(0x8F);

	epaper_cmd(0x50); // VCOM_AND_DATA_INTERVAL_SETTING
	epaper_write(0xF0);
	epaper_cmd(0x61); // RESOLUTION_SETTING
	epaper_write(EPD_WIDTH); // width: 104
	epaper_write(EPD_HEIGHT >> 8); // height: 212
	epaper_write(EPD_HEIGHT & 0xFF);
}

static void epaper_clear(void) {
	uint32_t w = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
	uint32_t h = EPD_HEIGHT;

	//send black data
	spi_activate(1);

	epaper_cmd_raw(0x10);
	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {
			epaper_write_raw(0xf0);
		}
	}
	epaper_cmd_raw(0x92);

	//send red data
	epaper_cmd_raw(0x13);
	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {
			epaper_write_raw(0x0f);
		}
	}
	epaper_cmd_raw(0x92);

	spi_activate(0);
	epaper_on();
}

#define SPI_CLK_DIVIDE_TEST 64

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	gpio_init();
	gpio_config(EPD_RST_PIN, GPIO_OUTPUT);
	gpio_config(EPD_DC_PIN, GPIO_OUTPUT);
	gpio_config(EPD_CS_PIN, GPIO_OUTPUT);
	gpio_config(EPD_BUSY_PIN, GPIO_INPUT);
	gpio_write(EPD_CS_PIN, 1);

	spi_init(SPI_CLK_DIVIDE_TEST);
	spi_select(SPI_SELECT_0);
	epaper_init();
	epaper_clear();
	return 0;
}
