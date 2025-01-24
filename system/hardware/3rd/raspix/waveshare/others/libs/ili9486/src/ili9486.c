#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>
#include <ili9486/ili9486.h>

static int LCD_DC =	24;
static int LCD_CS	= 8;
static int LCD_RST = 25;
static int SPI_DIV = 2;

// Screen settings
#define LCD_SCREEN_WIDTH 		480
#define LCD_SCREEN_HEIGHT 		320

// Other define
#define SCREEN_VERTICAL_1		0
#define SCREEN_HORIZONTAL_1		1
#define SCREEN_VERTICAL_2		2
#define SCREEN_HORIZONTAL_2		3

uint16_t _lcd_buffer[LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT];
uint16_t LCD_HEIGHT = LCD_SCREEN_HEIGHT;
uint16_t LCD_WIDTH  = LCD_SCREEN_WIDTH;

static inline void delay(int32_t count) {
	proc_usleep(count);
}

/* LCD CONTROL */
static inline void lcd_spi_send(uint8_t byte) {
	bcm283x_spi_transfer16(byte);
}

/* Send command (char) to LCD  - OK */
static inline void lcd_write_commmand(uint8_t command) {
	bcm283x_gpio_write(LCD_DC, 0);
	lcd_spi_send(command);
	bcm283x_gpio_write(LCD_DC, 1);
}

/* Send Data (char) to LCD */
static inline void lcd_write_data(uint8_t Data) {
	lcd_spi_send(Data);
}

/* Reset LCD */
static inline void lcd_reset( void ) {
	bcm283x_gpio_write(LCD_RST, 0);
	delay(200);
	bcm283x_gpio_write(LCD_RST, 1);
	delay(200);
}

static inline void lcd_start(void) {
	bcm283x_gpio_write(LCD_CS, 0);
	bcm283x_spi_activate(1);
}

static inline void lcd_end(void) {
	bcm283x_spi_activate(0);
	bcm283x_gpio_write(LCD_CS, 1);
}

/*Ser rotation of the screen - changes x0 and y0*/
static inline void lcd_set_rotation(uint8_t rotation) {
	lcd_write_commmand(0x36);
	delay(100);

	switch(rotation) {
		case SCREEN_VERTICAL_1:
			lcd_write_data(0x48);
			LCD_WIDTH = 320;
			LCD_HEIGHT = 480;
			break;
		case SCREEN_HORIZONTAL_1:
			lcd_write_data(0x28);
			LCD_WIDTH  = 480;
			LCD_HEIGHT = 320;
			break;
		case SCREEN_VERTICAL_2:
			lcd_write_data(0x98);
			LCD_WIDTH  = 320;
			LCD_HEIGHT = 480;
			break;
		case SCREEN_HORIZONTAL_2:
			lcd_write_data(0xF8);
			LCD_WIDTH  = 480;
			LCD_HEIGHT = 320;
			break;
		default:
			//EXIT IF SCREEN ROTATION NOT VALID!
			break;
	}
}

static inline void lcd_brightness(uint8_t brightness) {
	// chyba trzeba wczesniej zainicjalizowac - rejestr 0x53
	lcd_write_commmand(0x51); // byc moze 2 bajty?
	lcd_write_data(brightness); // byc moze 2 bajty
}

static inline void lcd_show(void) {
	int i, j, m = 0;
	lcd_write_commmand(0x2B);
	lcd_write_data(0x00);
	lcd_write_data(0x00);
	lcd_write_data(0x01);
	lcd_write_data(0x3F);

	lcd_write_commmand(0x2A);
	lcd_write_data(0x00);
	lcd_write_data(0x00);
	lcd_write_data(0x01);
	lcd_write_data(0xE0);

	lcd_write_commmand(0x2C); // Memory write?

#define SPI_FIFO_SIZE  64
	uint8_t c8[SPI_FIFO_SIZE];

	for ( i = 0 ; i < 30  ; i ++ ) {
		uint16_t *tx_data = (uint16_t*)&_lcd_buffer[5120*i];
		int32_t data_sz = 5120;
		for( j=0; j<data_sz; j++)  {
			uint16_t color = tx_data[j];
			c8[m++] = (color >> 8) & 0xff;
			c8[m++] = (color) & 0xff;
			if(m >= SPI_FIFO_SIZE) {
				m = 0;
				bcm283x_spi_send_recv(c8, NULL, SPI_FIFO_SIZE);
			}
		}
	}
}

void ili9486_flush(const void* buf, uint32_t size) {
	if(size < LCD_WIDTH * LCD_HEIGHT* 4)
		return;

	uint32_t *src = (uint32_t*)buf;
	uint32_t sz = LCD_HEIGHT*LCD_WIDTH;
	uint32_t i;

	for (i = 0; i < sz; i++) {
		register uint32_t s = src[i];
		register uint8_t r = (s >> 16) & 0xff;
		register uint8_t g = (s >> 8)  & 0xff;
		register uint8_t b = s & 0xff;
		_lcd_buffer[i] = ((r >> 3) <<11) | ((g >> 2) << 5) | (b >> 3);
	}

	bcm283x_spi_set_div(SPI_DIV);
	lcd_start();
	lcd_show();
	lcd_end();
}

void ili9486_init(int pin_rs, int pin_cs, int pin_rst, int cdiv) {
	LCD_DC = pin_rs;
	LCD_CS = pin_cs;
	LCD_RST = pin_rst;
	bcm283x_gpio_init();
	bcm283x_gpio_config(LCD_DC, GPIO_OUTPUT);
	bcm283x_gpio_config(LCD_CS, GPIO_OUTPUT);
	bcm283x_gpio_config(LCD_RST, GPIO_OUTPUT);

	lcd_reset();
	if(cdiv > 0)
		SPI_DIV = cdiv;
	bcm283x_spi_set_div(SPI_DIV);
	bcm283x_spi_select(SPI_SELECT_0);

	lcd_start();

	lcd_write_commmand(0x28); // Display OFF

	lcd_write_commmand(0x3A); // Interface Pixel Format
	lcd_write_data(0x55);	// 16 bit/pixel

	lcd_write_commmand(0xC2); // Power Control 3 (For Normal Mode)
	lcd_write_data(0x44);    // Cos z napieciem

	lcd_write_commmand(0xC5); // VCOM Control
	lcd_write_data(0x00);  // const
	lcd_write_data(0x00);  // nVM ? 0x48
	lcd_write_data(0x00);  // VCOM voltage ref
	lcd_write_data(0x00);  // VCM out

	lcd_write_commmand(0xE0); // PGAMCTRL(Positive Gamma Control)
	lcd_write_data(0x0F);
	lcd_write_data(0x1F);
	lcd_write_data(0x1C);
	lcd_write_data(0x0C);
	lcd_write_data(0x0F);
	lcd_write_data(0x08);
	lcd_write_data(0x48);
	lcd_write_data(0x98);
	lcd_write_data(0x37);
	lcd_write_data(0x0A);
	lcd_write_data(0x13);
	lcd_write_data(0x04);
	lcd_write_data(0x11);
	lcd_write_data(0x0D);
	lcd_write_data(0x00);

	lcd_write_commmand(0xE1); // NGAMCTRL (Negative Gamma Correction)
	lcd_write_data(0x0F);
	lcd_write_data(0x32);
	lcd_write_data(0x2E);
	lcd_write_data(0x0B);
	lcd_write_data(0x0D);
	lcd_write_data(0x05);
	lcd_write_data(0x47);
	lcd_write_data(0x75);
	lcd_write_data(0x37);
	lcd_write_data(0x06);
	lcd_write_data(0x10);
	lcd_write_data(0x03);
	lcd_write_data(0x24);
	lcd_write_data(0x20);
	lcd_write_data(0x00);

	lcd_write_commmand(0x11); // sw reset, wakeup
	delay(150000);

	lcd_write_commmand(0x20); // Display Inversion OFF   RPi LCD (A)
	//lcd_write_commmand(0x21); // Display Inversion ON    RPi LCD (B)

	//lcd_write_commmand(0x36); // Memory Access Control
	//lcd_write_data(0x48);
	lcd_set_rotation(SCREEN_HORIZONTAL_1);
	lcd_write_commmand(0x29); // Display ON
	delay(150000);
	lcd_end();
}