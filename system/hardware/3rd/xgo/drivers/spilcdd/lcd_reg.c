#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vdevice.h>
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>

#include "lcd_reg.h"

static int LCD_DC =	25;
static int LCD_BL	= 0;
static int LCD_RST = 27;
static int SPI_DIV = 8;

uint16_t _lcd_buffer[LCD_WIDTH * LCD_HEIGHT];

static inline void delay(int32_t count) {
	proc_usleep(count);
}

/* LCD CONTROL */
static inline void lcd_cs(int en){
	bcm283x_spi_activate(en);
	delay(1);
}

static inline void lcd_spi_send(uint8_t byte) {
	lcd_cs(1);
	bcm283x_spi_transfer(byte);
	lcd_cs(0);
}

/* Send command (char) to LCD  - OK */
static inline void lcd_write_commmand(uint8_t command) {
	bcm283x_gpio_write(LCD_DC, 0);
	lcd_spi_send(command);
}

/* Send Data (char) to LCD */
static inline void lcd_write_data(uint8_t Data) {
	bcm283x_gpio_write(LCD_DC, 1);
	lcd_spi_send(Data);
}

/* Reset LCD */
static inline void lcd_reset( void ) {
	bcm283x_gpio_write(LCD_RST, 0);
	delay(20000);
	bcm283x_gpio_write(LCD_RST, 1);
	delay(20000);
}

/*Ser rotation of the screen - changes x0 and y0*/
static inline void lcd_set_rotation(uint8_t rotation) {
}

static inline void lcd_brightness(uint8_t brightness) {
	// chyba trzeba wczesniej zainicjalizowac - rejestr 0x53
	//lcd_write_commmand(0x51); // byc moze 2 bajty?
	//lcd_write_data(brightness); // byc moze 2 bajty
}

static inline void lcd_set_window(int startX, int startY, int endX, int endY){
	lcd_write_commmand(0x2A);
	lcd_write_data(startX>>8);
	lcd_write_data(startX&0xFF);
	lcd_write_data(endX>>8);
	lcd_write_data(endX&0xFF);

	lcd_write_commmand(0x2B);
	lcd_write_data(startY>>8);
	lcd_write_data(startY&0xFF);
	lcd_write_data(endY>>8);
	lcd_write_data(endY&0xFF);

	lcd_write_commmand(0x2C); // Memory write?
}

static inline void lcd_show(void) {
	int i, j;
	uint8_t temp[64] = {0};
	memset(temp, 0xFF, 32);
	lcd_write_commmand(0x36);
	lcd_write_data(0x70);
	lcd_set_window(0, 0, LCD_WIDTH, LCD_HEIGHT);

	bcm283x_gpio_write(LCD_DC, 1);
	bcm283x_spi_activate(1);
	uint8_t* p = (uint8_t*)_lcd_buffer;
	for(i = 0; i < LCD_WIDTH * LCD_HEIGHT * 2; i+=64) {
		bcm283x_spi_send_recv(p, NULL, 64);
		p+=64;
	}

	bcm283x_spi_activate(0);
}

void lcd_flush(const void* buf, uint32_t size) {
	if(size < LCD_WIDTH * LCD_HEIGHT* 4)
		return;

	uint32_t sz = LCD_HEIGHT*LCD_WIDTH;
	uint8_t *src = (uint8_t*)buf;
	uint16_t *dst = _lcd_buffer;
	uint32_t i;

	for (i = 0; i < sz; i++) {
        *dst++ = (src[2] & 0xF8) | (src[1] >> 5) | ((src[1] &0x1C) << 11)  | ((src[0] & 0xF8) << 5);
		src+=4;
	}

	bcm283x_spi_set_div(SPI_DIV);
	lcd_show();
}

void lcd_init(int pin_rst, int pin_dc, int pin_bl, int cdiv) {
	LCD_RST = pin_rst;
	LCD_DC = pin_dc;
	LCD_BL = pin_bl;
	bcm283x_gpio_init();
	bcm283x_gpio_config(LCD_RST, GPIO_OUTPUT);
	bcm283x_gpio_config(LCD_DC, GPIO_OUTPUT);
	//bcm283x_gpio_config(LCD_BL, GPIO_OUTPUT);
	//bcm283x_gpio_write(LCD_BL, 1);

	lcd_reset();
	if(cdiv > 0)
		SPI_DIV = cdiv;
	bcm283x_spi_set_div(SPI_DIV);
	bcm283x_spi_select(SPI_SELECT_0);

    lcd_write_commmand(0x36);
    lcd_write_data(0x00) ;

    lcd_write_commmand(0x3A); 
    lcd_write_data(0x05);

    lcd_write_commmand(0x21); 

    lcd_write_commmand(0x2A);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x01);
    lcd_write_data(0x3F);

    lcd_write_commmand(0x2B);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0x00);
    lcd_write_data(0xEF);

    lcd_write_commmand(0xB2);
    lcd_write_data(0x0C);
    lcd_write_data(0x0C);
    lcd_write_data(0x00);
    lcd_write_data(0x33);
    lcd_write_data(0x33);

    lcd_write_commmand(0xB7);
    lcd_write_data(0x35); 

    lcd_write_commmand(0xBB);
    lcd_write_data(0x1F);

    lcd_write_commmand(0xC0);
    lcd_write_data(0x2C);

    lcd_write_commmand(0xC2);
    lcd_write_data(0x01);

    lcd_write_commmand(0xC3);
    lcd_write_data(0x12);   

    lcd_write_commmand(0xC4);
    lcd_write_data(0x20);

    lcd_write_commmand(0xC6);
    lcd_write_data(0x0F); 

    lcd_write_commmand(0xD0);
    lcd_write_data(0xA4);
    lcd_write_data(0xA1);

    lcd_write_commmand(0xE0);
    lcd_write_data(0xD0);
    lcd_write_data(0x08);
    lcd_write_data(0x11);
    lcd_write_data(0x08);
    lcd_write_data(0x0C);
    lcd_write_data(0x15);
    lcd_write_data(0x39);
    lcd_write_data(0x33);
    lcd_write_data(0x50);
    lcd_write_data(0x36);
    lcd_write_data(0x13);
    lcd_write_data(0x14);
    lcd_write_data(0x29);
    lcd_write_data(0x2D);

    lcd_write_commmand(0xE1);
    lcd_write_data(0xD0);
    lcd_write_data(0x08);
    lcd_write_data(0x10);
    lcd_write_data(0x08);
    lcd_write_data(0x06);
    lcd_write_data(0x06);
    lcd_write_data(0x39);
    lcd_write_data(0x44);
    lcd_write_data(0x51);
    lcd_write_data(0x0B);
    lcd_write_data(0x16);
    lcd_write_data(0x14);
    lcd_write_data(0x2F);
    lcd_write_data(0x31);
    lcd_write_commmand(0x21);

    lcd_write_commmand(0x11);

    lcd_write_commmand(0x29);
}
