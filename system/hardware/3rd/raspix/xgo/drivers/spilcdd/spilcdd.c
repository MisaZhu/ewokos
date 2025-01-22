#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <fb/fb.h>
#include <ewoksys/vdevice.h>
#include <arch/bcm283x/spi.h>
#include <fbd/fbd.h>
#include "lcd_reg.h"

static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	uint32_t sz = 4 * g->w * g->h;
	lcd_flush(g->buffer, sz);
	return sz;
}

static fbinfo_t* get_info(void) {
	static fbinfo_t fbinfo;
	fbinfo.width = LCD_WIDTH;
	fbinfo.height = LCD_HEIGHT;
	fbinfo.depth = 32;
	return &fbinfo;
}

static int32_t init(uint32_t w, uint32_t h, uint32_t dep) {
	(void)w;
	(void)h;
	(void)dep;
	return 0;
}

/*LCD_RS	Instruction/Data Register selection
  LCD_CS    LCD chip selection, low active
  LCD_RST   LCD reset
  */
int main(int argc, char** argv) {
	int lcd_rst = 27;
	int lcd_dc = 25;
	int lcd_bl = 0;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";

	if(argc > 4) {
		lcd_rst = atoi(argv[2]);
		lcd_dc = atoi(argv[3]);
		lcd_bl = atoi(argv[4]);
	}

	bcm283x_spi_init();
	lcd_init(lcd_rst, lcd_dc, lcd_bl, 4);

	fbd_t fbd;
	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	return fbd_run(&fbd, mnt_point, LCD_WIDTH, LCD_HEIGHT, G_ROTATE_NONE);
}
