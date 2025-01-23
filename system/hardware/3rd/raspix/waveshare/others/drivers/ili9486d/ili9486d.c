#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <fb/fb.h>
#include <ewoksys/vdevice.h>
#include <ili9486/ili9486.h>
#include <fbd/fbd.h>

static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	uint32_t sz = 4 * g->w * g->h;
	ili9486_flush(g->buffer, sz);
	return sz;
}

static fbinfo_t* get_info(void) {
	static fbinfo_t fbinfo;
	memset(&fbinfo, 0, sizeof(fbinfo_t));
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
	int lcd_rs = 24;
	int lcd_cs = 8;
	int lcd_rst = 25;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";

	if(argc > 4) {
		lcd_rs = atoi(argv[2]);
		lcd_cs = atoi(argv[3]);
		lcd_rst = atoi(argv[4]);
	}

	bcm283x_spi_init();
	ili9486_init(lcd_rs, lcd_cs, lcd_rst, 2);

	fbd_t fbd;
	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	return fbd_run(&fbd, mnt_point, LCD_HEIGHT, LCD_WIDTH, G_ROTATE_NONE);
}
