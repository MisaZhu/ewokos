#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>

#define LCD_DC		24
#define LCD_CS		8
#define LCD_RST		25

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
	usleep(count);
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
static inline void LCD_reset( void ) {
	bcm283x_gpio_write(LCD_CS, 0);
	delay(150);
	bcm283x_gpio_write(LCD_RST, 0);
	delay(200);
	bcm283x_gpio_write(LCD_RST, 1);
	delay(200);
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
	int i, j;
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

	for ( i = 0 ; i < 30  ; i ++ ) {
		uint8_t *tx_data = (uint8_t*)&_lcd_buffer[5120*i];
		int32_t data_sz = 2 * 5120;
		for( j=0; j<data_sz; j++)  {
			bcm283x_spi_transfer(tx_data[j]);
		}
	}
}

#define MIN(x, y) ((x)<(y)? (x):(y))

int  do_flush(const void* buf, uint32_t size) {
	if(size < LCD_WIDTH * LCD_HEIGHT* 4)
		return -1;

	uint32_t *src = (uint32_t*)buf;
	uint32_t sz = LCD_HEIGHT*LCD_WIDTH;
	uint32_t i;

	for (i = 0; i < sz; i++) {
		register uint32_t s = src[i];
		register uint8_t r = (s >> 16) & 0xff;
		register uint8_t g = (s >> 8)  & 0xff;
		register uint8_t b = s & 0xff;
		//_lcd_buffer[i] = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
		_lcd_buffer[i] = ((r >> 3) <<10) | ((g >> 3) << 5) | (b >> 3);
	}

	lcd_show();
	return 0;
}

void lcd_init(void) {
	bcm283x_gpio_init();
	bcm283x_gpio_config(LCD_DC, GPIO_OUTPUT);
	bcm283x_gpio_config(LCD_CS, GPIO_OUTPUT);
	bcm283x_gpio_config(LCD_RST, GPIO_OUTPUT);

	bcm283x_spi_init(2);
	bcm283x_spi_select(1);
	bcm283x_spi_activate(1);
	LCD_reset();
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

	lcd_write_commmand(0x36); // Memory Access Control
	lcd_write_data(0x48);

	lcd_write_commmand(0x29); // Display ON
	delay(150000);
	lcd_set_rotation(SCREEN_HORIZONTAL_1);
}

