#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/vdevice.h>
#include <arch/bcm283x/gpio.h>
#include <arch/bcm283x/spi.h>

#define LCD_DC		24
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
volatile uint16_t LCD_HEIGHT = LCD_SCREEN_HEIGHT;
volatile uint16_t LCD_WIDTH  = LCD_SCREEN_WIDTH;

static int ili9486write(int fd, 
		int from_pid,
		fsinfo_t* info,
		const void* buf,
		int size,
		int offset,
		void* p) {
		
	(void)fd;
	(void)from_pid;
	(void)info;
	(void)buf;
	(void)offset;
	(void)p;
	return size;
}

static inline void delay(int32_t count) {
	while(count > 0) count--;
}

/* LCD CONTROL */
static inline void LCD_spiSendByte(uint8_t byte) {
	bcm283x_spi_transfer(byte);
}

/* Send command (char) to LCD  - OK */
static inline void LCD_writeCommand(uint8_t command) {
	bcm283x_gpio_write(LCD_DC, 0);
	LCD_spiSendByte(command);
}

/* Send Data (char) to LCD */
static inline void LCD_writeData(uint8_t Data) {
	bcm283x_gpio_write(LCD_DC, 1);
	LCD_spiSendByte(Data);
}

/* Reset LCD */
static inline void LCD_reset( void ) {
	bcm283x_gpio_write(LCD_RST, 0);
	delay(200000);
	bcm283x_gpio_write(LCD_RST, 1);
	delay(200000);
}

/*Ser rotation of the screen - changes x0 and y0*/
void LCD_setRotation(uint8_t rotation) {
	LCD_writeCommand(0x36);
	delay(100);

	switch(rotation) {
		case SCREEN_VERTICAL_1:
			LCD_writeData(0x48);
			LCD_WIDTH = 320;
			LCD_HEIGHT = 480;
			break;
		case SCREEN_HORIZONTAL_1:
			LCD_writeData(0x28);
			LCD_WIDTH  = 480;
			LCD_HEIGHT = 320;
			break;
		case SCREEN_VERTICAL_2:
			LCD_writeData(0x98);
			LCD_WIDTH  = 320;
			LCD_HEIGHT = 480;
			break;
		case SCREEN_HORIZONTAL_2:
			LCD_writeData(0xF8);
			LCD_WIDTH  = 480;
			LCD_HEIGHT = 320;
			break;
		default:
			//EXIT IF SCREEN ROTATION NOT VALID!
			break;
	}
}

void LCD_brightness(uint8_t brightness) {
	// chyba trzeba wczesniej zainicjalizowac - rejestr 0x53
	LCD_writeCommand(0x51); // byc moze 2 bajty?
	LCD_writeData(brightness); // byc moze 2 bajty
}

uint16_t setColor(uint8_t r, uint8_t g, uint8_t b) {
	uint16_t color = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
	return color;
}

void LCD_drawPixel(uint16_t X,uint16_t Y,uint16_t colour) {
	if((X >=LCD_WIDTH) || (Y >=LCD_HEIGHT)) return;	//OUT OF BOUNDS!

	LCD_writeCommand(0x2A); // Column Address Set
	LCD_writeData( (uint8_t)( X >> 8 ) );
	LCD_writeData( (uint8_t)( X ) );
	LCD_writeData( (uint8_t)( X >> 8 ) );
	LCD_writeData( (uint8_t)( X ) );

	LCD_writeCommand(0x2B); // Page Address Set
	LCD_writeData( (uint8_t)( Y >> 8 ) );
	LCD_writeData( (uint8_t)( Y ) );
	LCD_writeData( (uint8_t)( Y >> 8 ) );
	LCD_writeData( (uint8_t)( Y ) );

	LCD_writeCommand(0x2C); // Memory write

	LCD_writeData( (uint8_t)(colour >> 8) ); // set color
	LCD_writeData( (uint8_t)(colour) );
}

void LCD_fill(uint16_t color) {
	int i, j;
	for ( i = 0 ; i < LCD_WIDTH*LCD_HEIGHT ; i ++ ) {
		_lcd_buffer[i] = color;
	}

	LCD_writeCommand(0x2B);
	LCD_writeData(0x00);
	LCD_writeData(0x00);
	LCD_writeData(0x01);
	LCD_writeData(0x3F);

	LCD_writeCommand(0x2C);
	LCD_writeData(0x00);
	LCD_writeData(0x00);
	LCD_writeData(0x01);
	LCD_writeData(0xE0);

	LCD_writeCommand(0x2C); // Memory write?

	for ( i = 0 ; i < 30  ; i ++ ) {
		uint8_t *tx_data = (uint8_t*)&_lcd_buffer[5120*i];
		int32_t data_sz = 2 * 5120;
		for( j=0; j<data_sz; j++) 
			bcm283x_spi_transfer(tx_data[j]);
	}
}

void LCD_showBuffer(void) {
	int i, j;

	LCD_writeCommand(0x2B);
	LCD_writeData(0x00);
	LCD_writeData(0x00);
	LCD_writeData(0x01);
	LCD_writeData(0x3F);

	LCD_writeCommand(0x2A);
	LCD_writeData(0x00);
	LCD_writeData(0x00);
	LCD_writeData(0x01);
	LCD_writeData(0xE0);

	LCD_writeCommand(0x2C); // Memory write?

	for ( i = 0 ; i < 30  ; i ++ ) {
		uint8_t *tx_data = (uint8_t*)&_lcd_buffer[5120*i];
		int32_t data_sz = 2 * 5120;
		for( j=0; j<data_sz; j++) 
			bcm283x_spi_transfer(tx_data[j]);
	}
}


void LCD_clear(void) {
	LCD_fill(0x0ff0);
}

void LCD_bufferClear(uint16_t* buffer) {
	memset(buffer, 0, LCD_SCREEN_WIDTH * LCD_SCREEN_HEIGHT );
}



static int init(void) {
	bcm283x_gpio_init();
	bcm283x_gpio_config(LCD_DC, GPIO_OUTPUT);
	bcm283x_gpio_config(LCD_RST, GPIO_OUTPUT);

	LCD_reset();

	LCD_writeCommand(0x28); // Display OFF

	LCD_writeCommand(0x3A); // Interface Pixel Format
	LCD_writeData(0x55);	// 16 bit/pixel

	LCD_writeCommand(0xC2); // Power Control 3 (For Normal Mode)
	LCD_writeData(0x44);    // Cos z napieciem

	LCD_writeCommand(0xC5); // VCOM Control
	LCD_writeData(0x00);  // const
	LCD_writeData(0x00);  // nVM ? 0x48
	LCD_writeData(0x00);  // VCOM voltage ref
	LCD_writeData(0x00);  // VCM out

	LCD_writeCommand(0xE0); // PGAMCTRL(Positive Gamma Control)
	LCD_writeData(0x0F);
	LCD_writeData(0x1F);
	LCD_writeData(0x1C);
	LCD_writeData(0x0C);
	LCD_writeData(0x0F);
	LCD_writeData(0x08);
	LCD_writeData(0x48);
	LCD_writeData(0x98);
	LCD_writeData(0x37);
	LCD_writeData(0x0A);
	LCD_writeData(0x13);
	LCD_writeData(0x04);
	LCD_writeData(0x11);
	LCD_writeData(0x0D);
	LCD_writeData(0x00);

	LCD_writeCommand(0xE1); // NGAMCTRL (Negative Gamma Correction)
	LCD_writeData(0x0F);
	LCD_writeData(0x32);
	LCD_writeData(0x2E);
	LCD_writeData(0x0B);
	LCD_writeData(0x0D);
	LCD_writeData(0x05);
	LCD_writeData(0x47);
	LCD_writeData(0x75);
	LCD_writeData(0x37);
	LCD_writeData(0x06);
	LCD_writeData(0x10);
	LCD_writeData(0x03);
	LCD_writeData(0x24);
	LCD_writeData(0x20);
	LCD_writeData(0x00);

	LCD_writeCommand(0x11);	// Exit sleep - Sleep OUT
	delay(120000);	 		// 120 ms wait

	LCD_writeCommand(0x20); // Display Inversion OFF   RPi LCD (A)
	//LCD_writeCommand(0x21); // Display Inversion ON    RPi LCD (B)

	LCD_writeCommand(0x36); // Memory Access Control
	LCD_writeData(0x48);

	LCD_writeCommand(0x29); // Display ON
	delay(150000);

	LCD_setRotation(SCREEN_HORIZONTAL_2);
	LCD_clear();
	return 0;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb1";
	init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "ili9486");
	dev.write = ili9486write;

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	return 0;
}
