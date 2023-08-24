#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <fbd/fbd.h>

static uint32_t LCD_HEIGHT = 240;
static uint32_t LCD_WIDTH = 240;


int  do_flush(const void* buf, uint32_t size);
void lcd_init(uint32_t w, uint32_t h, uint32_t rot);


static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	(void)rotate;
	uint32_t sz = 4 * fbinfo->width * fbinfo->height;
	if(size < sz || fbinfo->depth != 32)
		return -1;

	do_flush(buf, size);
	return size;
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

int main(int argc, char** argv) {
	uint32_t w=240, h=240, rot=0;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";
	if(argc > 2) {
		rot = atoi(argv[2]);
	}
	LCD_HEIGHT = h;
	LCD_WIDTH = w;
	lcd_init(w, h, rot);

	fbd_t fbd;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	return fbd_run(&fbd, mnt_point, LCD_WIDTH, LCD_HEIGHT, rot);
}
