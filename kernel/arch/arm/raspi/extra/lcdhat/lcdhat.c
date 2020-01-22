#include <kstring.h>
#include <kernel/system.h>
#include "../lib/gpio_arch.h"
#include "../lib/spi_arch.h"
#include "lcdhat.h"

#define UBYTE   uint8_t
#define UWORD   uint16_t
#define UDOUBLE uint32_t

#define LCD_CS   8
#define LCD_RST  27
#define LCD_DC   25
#define LCD_BL   24

#define DEV_Delay_ms(x) _delay_usec((x)*1000)
#define DEV_Digital_Write gpio_arch_write

#define LCD_CS_0		DEV_Digital_Write(LCD_CS,0)
#define LCD_CS_1		DEV_Digital_Write(LCD_CS,1)

#define LCD_RST_0		DEV_Digital_Write(LCD_RST,0)
#define LCD_RST_1		DEV_Digital_Write(LCD_RST,1)

#define LCD_DC_0		DEV_Digital_Write(LCD_DC,0)
#define LCD_DC_1		DEV_Digital_Write(LCD_DC,1)

#define LCD_BL_0		DEV_Digital_Write(LCD_BL,0)
#define LCD_BL_1		DEV_Digital_Write(LCD_BL,1)

#define LCD_WIDTH_Byte 240

#define HORIZONTAL 0
#define VERTICAL   1

typedef struct{
	UWORD WIDTH;
	UWORD HEIGHT;
	UBYTE SCAN_DIR;
}LCD_ATTRIBUTES;
static LCD_ATTRIBUTES LCD;

static inline void DEV_SPI_Write(UBYTE* data, uint32_t sz) {
	spi_arch_activate(1);
	for(uint32_t i=0; i<sz; i++)
		spi_arch_transfer(data[i]);
	spi_arch_activate(0);
}

/******************************************************************************
function :	Hardware reset
parameter:
 ******************************************************************************/
static void LCD_Reset(void) {
	LCD_RST_1;
	DEV_Delay_ms(100);
	LCD_RST_0;
	DEV_Delay_ms(100);
	LCD_RST_1;
	DEV_Delay_ms(100);
}

/******************************************************************************
function :	send command
parameter:
Reg : Command register
 ******************************************************************************/
static void LCD_SendCommand(UBYTE Reg) {
	LCD_DC_0;
	// LCD_CS_0;
	DEV_SPI_Write(&Reg, 1);
	// LCD_CS_1;
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
 ******************************************************************************/
static void LCD_SendData_8Bit(UBYTE Data) {
	LCD_DC_1;
	// LCD_CS_0;
	DEV_SPI_Write(&Data, 1);
	// LCD_CS_1;
}

/******************************************************************************
function :	send data
parameter:
    Data : Write data
 ******************************************************************************/
/*static void LCD_SendData_16Bit(UWORD Data) {
	LCD_DC_1;
	// LCD_CS_0;
	DEV_SPI_WriteByte((Data >> 8) & 0xFF);
	DEV_SPI_WriteByte(Data & 0xFF);
	// LCD_CS_1;
}
*/

/********************************************************************************
function:	Set the resolution and scanning method of the screen
parameter:
		Scan_dir:   Scan direction
 ********************************************************************************/
static void LCD_SetAttributes(UBYTE Scan_dir) {
	//Get the screen scan direction
	LCD.SCAN_DIR = Scan_dir;
	UBYTE MemoryAccessReg = 0x00;

	//Get GRAM and LCD width and height
	if(Scan_dir == HORIZONTAL) {
		LCD.HEIGHT	= LCD_HEIGHT;
		LCD.WIDTH   = LCD_WIDTH;
		MemoryAccessReg = 0X70;
	} else {
		LCD.HEIGHT	= LCD_WIDTH;
		LCD.WIDTH   = LCD_HEIGHT;
		MemoryAccessReg = 0X00;
	}

	// Set the read / write scan direction of the frame memory
	LCD_SendCommand(0x36); //MX, MY, RGB mode
	LCD_SendData_8Bit(MemoryAccessReg);	//0x08 set RGB
}

/******************************************************************************
function :	Initialize the lcd register
parameter:
 ******************************************************************************/
static void LCD_InitReg(void) {
	LCD_SendCommand(0x11); 
	DEV_Delay_ms(120);
	// LCD_SendCommand(0x36);
	// LCD_SendData_8Bit(0x00);

	LCD_SendCommand(0x3A); 
	LCD_SendData_8Bit(0x05);

	LCD_SendCommand(0xB2);
	LCD_SendData_8Bit(0x0C);
	LCD_SendData_8Bit(0x0C);
	LCD_SendData_8Bit(0x00);
	LCD_SendData_8Bit(0x33);
	LCD_SendData_8Bit(0x33); 

	LCD_SendCommand(0xB7); 
	LCD_SendData_8Bit(0x35);  

	LCD_SendCommand(0xBB);
	LCD_SendData_8Bit(0x37);

	LCD_SendCommand(0xC0);
	LCD_SendData_8Bit(0x2C);

	LCD_SendCommand(0xC2);
	LCD_SendData_8Bit(0x01);

	LCD_SendCommand(0xC3);
	LCD_SendData_8Bit(0x12);   

	LCD_SendCommand(0xC4);
	LCD_SendData_8Bit(0x20);  

	LCD_SendCommand(0xC6); 
	LCD_SendData_8Bit(0x0F);    

	LCD_SendCommand(0xD0); 
	LCD_SendData_8Bit(0xA4);
	LCD_SendData_8Bit(0xA1);

	LCD_SendCommand(0xE0);
	LCD_SendData_8Bit(0xD0);
	LCD_SendData_8Bit(0x04);
	LCD_SendData_8Bit(0x0D);
	LCD_SendData_8Bit(0x11);
	LCD_SendData_8Bit(0x13);
	LCD_SendData_8Bit(0x2B);
	LCD_SendData_8Bit(0x3F);
	LCD_SendData_8Bit(0x54);
	LCD_SendData_8Bit(0x4C);
	LCD_SendData_8Bit(0x18);
	LCD_SendData_8Bit(0x0D);
	LCD_SendData_8Bit(0x0B);
	LCD_SendData_8Bit(0x1F);
	LCD_SendData_8Bit(0x23);

	LCD_SendCommand(0xE1);
	LCD_SendData_8Bit(0xD0);
	LCD_SendData_8Bit(0x04);
	LCD_SendData_8Bit(0x0C);
	LCD_SendData_8Bit(0x11);
	LCD_SendData_8Bit(0x13);
	LCD_SendData_8Bit(0x2C);
	LCD_SendData_8Bit(0x3F);
	LCD_SendData_8Bit(0x44);
	LCD_SendData_8Bit(0x51);
	LCD_SendData_8Bit(0x2F);
	LCD_SendData_8Bit(0x1F);
	LCD_SendData_8Bit(0x1F);
	LCD_SendData_8Bit(0x20);
	LCD_SendData_8Bit(0x23);

	LCD_SendCommand(0x21); 

	LCD_SendCommand(0x29);
}

/********************************************************************************
function :	Initialize the lcd
parameter:
 ********************************************************************************/
static void LCD_1in3_Init(UBYTE Scan_dir) {
	//Turn on the backlight
	LCD_BL_1;

	//Hardware reset
	LCD_Reset();
	//Set the resolution and scanning method of the screen
	LCD_SetAttributes(Scan_dir);

	//Set the initialization register
	LCD_InitReg();
}

/********************************************************************************
function:	Sets the start position and size of the display area
parameter:
		Xstart 	:   X direction Start coordinates
		Ystart  :   Y direction Start coordinates
		Xend    :   X direction end coordinates
Yend    :   Y direction end coordinates
 ********************************************************************************/
static void LCD_1in3_SetWindows(UWORD Xstart, UWORD Ystart, UWORD Xend, UWORD Yend) {
	//set the X coordinates
	LCD_SendCommand(0x2A);
	LCD_SendData_8Bit((Xstart >> 8) & 0xFF);
	LCD_SendData_8Bit(Xstart & 0xFF);
	LCD_SendData_8Bit(((Xend  - 1) >> 8) & 0xFF);
	LCD_SendData_8Bit((Xend  - 1) & 0xFF);

	//set the Y coordinates
	LCD_SendCommand(0x2B);
	LCD_SendData_8Bit((Ystart >> 8) & 0xFF);
	LCD_SendData_8Bit(Ystart & 0xFF);
	LCD_SendData_8Bit(((Yend  - 1) >> 8) & 0xFF);
	LCD_SendData_8Bit((Yend  - 1) & 0xFF);

	LCD_SendCommand(0X2C);
}

/******************************************************************************
function :	Clear screen
parameter:
 ******************************************************************************/
static void LCD_1in3_Clear(UWORD Color) {
	UWORD j;
	UWORD Image[LCD_WIDTH*LCD_HEIGHT];

	Color = ((Color<<8)&0xff00)|(Color>>8);

	for (j = 0; j < LCD_HEIGHT*LCD_WIDTH; j++) {
		Image[j] = Color;
	}

	LCD_1in3_SetWindows(0, 0, LCD_WIDTH, LCD_HEIGHT);
	LCD_DC_1;
	DEV_SPI_Write((uint8_t *)Image, LCD_WIDTH*LCD_HEIGHT*2);
}

void lcd_init(void) {
	gpio_arch_config(LCD_CS, 1);
	gpio_arch_config(LCD_RST, 1);
	gpio_arch_config(LCD_DC, 1);
	gpio_arch_config(LCD_BL, 1);

	spi_arch_init(4);
	spi_arch_select(1);

	LCD_1in3_Init(HORIZONTAL);
	LCD_1in3_Clear(0x0);
}

void  lcd_flush(graph_t* g) {
	if(g->w != LCD_WIDTH  || g->h !=  LCD_HEIGHT)
		return;

	LCD_1in3_SetWindows(0, 0, LCD_WIDTH, LCD_HEIGHT);
	LCD_DC_1;
	spi_arch_activate(1);

	uint32_t *src = (uint32_t*)g->buffer;
	uint32_t sz = LCD_HEIGHT*LCD_WIDTH;
	UWORD i;
	UWORD color;

	for (i = 0; i < sz; i++) {
		uint32_t s = src[i];
		uint8_t b = (s >> 16) & 0xff;
		uint8_t g = (s >> 8)  & 0xff;
		uint8_t r = s & 0xff;
		color = ((r >> 3) <<11) | ((g >> 3) << 6) | (b >> 3);
		//color = ((color<<8)&0xff00)|(color>>8);
		uint8_t* p = (uint8_t*)&color;
		spi_arch_transfer(p[1]);
		spi_arch_transfer(p[0]);
	}

	spi_arch_activate(0);
}

