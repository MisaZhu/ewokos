#include "arch/bcm283x/gpio.h"
#include "arch/bcm283x/spi.h"
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
void epaper_reset(void) {
	gpio_write(EPD_RST_PIN, 1);
	wait_msec(200);
	gpio_write(EPD_RST_PIN, 0);
	wait_msec(200);
	gpio_write(EPD_RST_PIN, 1);
	wait_msec(200);
}

void epaper_cmd(uint8_t reg) {
	gpio_write(EPD_DC_PIN, 0);
	gpio_write(EPD_CS_PIN, 0);
	spi_transfer(reg);
	gpio_write(EPD_CS_PIN, 1);
}

void epaper_write(uint8_t data) {
	gpio_write(EPD_DC_PIN, 1);
	gpio_write(EPD_CS_PIN, 0);
	spi_transfer(data);
	gpio_write(EPD_CS_PIN, 1);
}

void epaper_wait(void) {
	while(gpio_read(EPD_BUSY_PIN) == 0) {
		wait_msec(50);
	}
}

void epaper_on(void) {
	epaper_cmd(0x12);		 //DISPLAY REFRESH
	wait_msec(10);
	epaper_wait();
}

void epaper_off(void) {
	epaper_cmd(0x02); // POWER_OFF
	epaper_wait();
	epaper_cmd(0x07); // DEEP_SLEEP
	epaper_write(0xA5); // check code
}

void epaper_init(void) {
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

void epaper_clear(void) {
	uint32_t w = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
	uint32_t h = EPD_HEIGHT;

	//send black data
	epaper_cmd(0x10);
	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {
			epaper_write(0x22);
		}
	}
	epaper_cmd(0x92);

	//send red data
	epaper_cmd(0x13);
	for (uint32_t j = 0; j < h; j++) {
		for (uint32_t i = 0; i < w; i++) {
			epaper_write(0x6);
		}
	}
	epaper_cmd(0x92);

	epaper_on();
}

/*
//V1
const unsigned char EPD_2IN13_lut_full_update[] = {
    0x22, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x11,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00
};
const unsigned char EPD_2IN13_lut_partial_update[] = {
    0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x0F, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static void epaper_reset(void)
{
    gpio_write(EPD_RST_PIN, 1);
    wait_msec(200);
    gpio_write(EPD_RST_PIN, 0);
    wait_msec(10);
    gpio_write(EPD_RST_PIN, 1);
    wait_msec(200);
}
static void epaper_cmd(uint8_t Reg)
{
    gpio_write(EPD_DC_PIN, 0);
    gpio_write(EPD_CS_PIN, 0);
    spi_transfer(Reg);
    gpio_write(EPD_CS_PIN, 1);
}
static void epaper_write(uint8_t Data)
{
    gpio_write(EPD_DC_PIN, 1);
    gpio_write(EPD_CS_PIN, 0);
    spi_transfer(Data);
    gpio_write(EPD_CS_PIN, 1);
}
void EPD_2IN13_ReadBusy(void)
{
    while(gpio_read(EPD_BUSY_PIN) == 1) {      //LOW: idle, HIGH: busy
        wait_msec(100);
    }
}
static void epaper_on(void)
{
    epaper_cmd(0x22); // DISPLAY_UPDATE_CONTROL_2
    epaper_write(0xC4);
    epaper_cmd(0X20);	// MASTER_ACTIVATION
    epaper_cmd(0xFF);	// TERMINATE_FRAME_READ_WRITE
    EPD_2IN13_ReadBusy();
}
static void epaper_set_windows(int x_start, int y_start, int x_end, int y_end)
{
    epaper_cmd(0x44);
    epaper_write((x_start >> 3) & 0xFF);
    epaper_write((x_end >> 3) & 0xFF);
    epaper_cmd(0x45);
    epaper_write(y_start & 0xFF);
    epaper_write((y_start >> 8) & 0xFF);
    epaper_write(y_end & 0xFF);
    epaper_write((y_end >> 8) & 0xFF);
}
static void epaper_set_cursor(int x, int y)
{
    epaper_cmd(0x4E);
    epaper_write((x >> 3) & 0xFF);
    epaper_cmd(0x4F);
    epaper_write(y & 0xFF);
    epaper_write((y >> 8) & 0xFF);
//    EPD_2IN13_ReadBusy();
}
void epaper_init(uint8_t Mode)
{
    epaper_reset();
    epaper_cmd(0x01); // DRIVER_OUTPUT_CONTROL
    epaper_write((EPD_HEIGHT - 1) & 0xFF);
    epaper_write(((EPD_HEIGHT - 1) >> 8) & 0xFF);
    epaper_write(0x00);			// GD = 0; SM = 0; TB = 0;
    epaper_cmd(0x0C);	// BOOSTER_SOFT_START_CONTROL
    epaper_write(0xD7);
    epaper_write(0xD6);
    epaper_write(0x9D);
    epaper_cmd(0x2C);	// WRITE_VCOM_REGISTER
    epaper_write(0xA8);     // VCOM 7C
    epaper_cmd(0x3A);	// SET_DUMMY_LINE_PERIOD
    epaper_write(0x1A);			// 4 dummy lines per gate
    epaper_cmd(0x3B);	// SET_GATE_TIME
    epaper_write(0x08);			// 2us per line
    epaper_cmd(0X3C);	// BORDER_WAVEFORM_CONTROL
    epaper_write(0x63);
    epaper_cmd(0X11);	// DATA_ENTRY_MODE_SETTING
    epaper_write(0x03);			// X increment; Y increment
    //set the look-up table register
    epaper_cmd(0x32);
    if(Mode == 1) {
        for (uint32_t i = 0; i < 30; i++) {
            epaper_write(EPD_2IN13_lut_full_update[i]);
        }
    } else {
        for (uint32_t i = 0; i < 30; i++) {
            epaper_write(EPD_2IN13_lut_partial_update[i]);
        }
    }
}
void epaper_clear(void)
{
    uint32_t Width, Height;
    Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
    Height = EPD_HEIGHT;
    epaper_set_windows(0, 0, EPD_WIDTH, EPD_HEIGHT);
    for (uint32_t j = 0; j < Height; j++) {
        epaper_set_cursor(0, j);
        epaper_cmd(0x24);
        for (uint32_t i = 0; i < Width; i++) {
            epaper_write(0X88);
        }
    }
    epaper_on();
}
//V2
const unsigned char epaper_lut_full_update[]= {
	0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
	0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
	0x80,0x60,0x40,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
	0x10,0x60,0x20,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7
	0x03,0x03,0x00,0x00,0x02,                       // TP0 A~D RP0
	0x09,0x09,0x00,0x00,0x02,                       // TP1 A~D RP1
	0x03,0x03,0x00,0x00,0x02,                       // TP2 A~D RP2
	0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
	0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
	0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
	0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6
	0x15,0x41,0xA8,0x32,0x30,0x0A,
};
const unsigned char epaper_lut_partial_update[]= { //20 bytes
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT0: BB:     VS 0 ~7
	0x80,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT1: BW:     VS 0 ~7
	0x40,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT2: WB:     VS 0 ~7
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT3: WW:     VS 0 ~7
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,             //LUT4: VCOM:   VS 0 ~7
	0x0A,0x00,0x00,0x00,0x00,                       // TP0 A~D RP0
	0x00,0x00,0x00,0x00,0x00,                       // TP1 A~D RP1
	0x00,0x00,0x00,0x00,0x00,                       // TP2 A~D RP2
	0x00,0x00,0x00,0x00,0x00,                       // TP3 A~D RP3
	0x00,0x00,0x00,0x00,0x00,                       // TP4 A~D RP4
	0x00,0x00,0x00,0x00,0x00,                       // TP5 A~D RP5
	0x00,0x00,0x00,0x00,0x00,                       // TP6 A~D RP6
	0x15,0x41,0xA8,0x32,0x30,0x0A,
};
static void epaper_reset(void)
{
	gpio_write(EPD_RST_PIN, 1);
	wait_msec(200);
	gpio_write(EPD_RST_PIN, 0);
	wait_msec(10);
	gpio_write(EPD_RST_PIN, 1);
	wait_msec(200);
}
static void epaper_cmd(uint8_t Reg)
{
	gpio_write(EPD_DC_PIN, 0);
	gpio_write(EPD_CS_PIN, 0);
	spi_transfer(Reg);
	gpio_write(EPD_CS_PIN, 1);
}
static void epaper_write(uint8_t Data)
{
	gpio_write(EPD_DC_PIN, 1);
	gpio_write(EPD_CS_PIN, 0);
	spi_transfer(Data);
	gpio_write(EPD_CS_PIN, 1);
}
void epaper_ReadBusy(void)
{
	while(gpio_read(EPD_BUSY_PIN) == 1) {      //LOW: idle, HIGH: busy
		wait_msec(100);
	}
}
static void epaper_on(void)
{
	epaper_cmd(0x22);
	epaper_write(0xC7);
	epaper_cmd(0x20);
	epaper_ReadBusy();
}
static void epaper_on_part(void)
{
	epaper_cmd(0x22);
	epaper_write(0x0C);
	epaper_cmd(0x20);
	epaper_ReadBusy();
}
void epaper_init(uint8_t Mode)
{
	uint32_t count;
	epaper_reset();
	if(Mode == 1) {
		epaper_ReadBusy();
		epaper_cmd(0x12); // soft reset
		epaper_ReadBusy();
		epaper_cmd(0x74); //set analog block control
		epaper_write(0x54);
		epaper_cmd(0x7E); //set digital block control
		epaper_write(0x3B);
		epaper_cmd(0x01); //Driver output control
		epaper_write(0xF9);
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_cmd(0x11); //data entry mode
		epaper_write(0x01);
		epaper_cmd(0x44); //set Ram-X address start/end position
		epaper_write(0x00);
		epaper_write(0x0F);    //0x0C-->(15+1)*8=128
		epaper_cmd(0x45); //set Ram-Y address start/end position
		epaper_write(0xF9);   //0xF9-->(249+1)=250
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_cmd(0x3C); //BorderWavefrom
		epaper_write(0x03);
		epaper_cmd(0x2C); //VCOM Voltage
		epaper_write(0x55); //
		epaper_cmd(0x03);
		epaper_write(epaper_lut_full_update[70]);
		epaper_cmd(0x04); //
		epaper_write(epaper_lut_full_update[71]);
		epaper_write(epaper_lut_full_update[72]);
		epaper_write(epaper_lut_full_update[73]);
		epaper_cmd(0x3A);     //Dummy Line
		epaper_write(epaper_lut_full_update[74]);
		epaper_cmd(0x3B);     //Gate time
		epaper_write(epaper_lut_full_update[75]);
		epaper_cmd(0x32);
		for(count = 0; count < 70; count++) {
			epaper_write(epaper_lut_full_update[count]);
		}
		epaper_cmd(0x4E);   // set RAM x address count to 0;
		epaper_write(0x00);
		epaper_cmd(0x4F);   // set RAM y address count to 0X127;
		epaper_write(0xF9);
		epaper_write(0x00);
		epaper_ReadBusy();
	} else {
		epaper_cmd(0x2C);     //VCOM Voltage
		epaper_write(0x26);
		epaper_ReadBusy();
		epaper_cmd(0x32);
		for(count = 0; count < 70; count++) {
			epaper_write(epaper_lut_partial_update[count]);
		}
		epaper_cmd(0x37);
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_write(0x40);
		epaper_write(0x00);
		epaper_write(0x00);
		epaper_cmd(0x22);
		epaper_write(0xC0);
		epaper_cmd(0x20);
		epaper_ReadBusy();
		epaper_cmd(0x3C); //BorderWavefrom
		epaper_write(0x01);
	}
}
void epaper_clear(void)
{
	uint32_t Width, Height;
	Width = (EPD_WIDTH % 8 == 0)? (EPD_WIDTH / 8 ): (EPD_WIDTH / 8 + 1);
	Height = EPD_HEIGHT;
	epaper_cmd(0x24);
	for (uint32_t j = 0; j < Height; j++) {
		for (uint32_t i = 0; i < Width; i++) {
			epaper_write(0X88);
		}
	}
	epaper_on();
}
*/

#define SPI_CLK_DIVIDE_TEST 128

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;
	spi_init(SPI_CLK_DIVIDE_TEST);
	epaper_init();
	epaper_clear();
	wait_msec(200);
	return 0;
}
