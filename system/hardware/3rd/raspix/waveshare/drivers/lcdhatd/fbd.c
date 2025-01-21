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


static uint32_t flush(const fbinfo_t* fbinfo, const graph_t* g) {
	uint32_t sz = 4 * g->w * g->h;
	do_flush(g->buffer, sz);
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

int main(int argc, char** argv) {
	uint32_t w=240, h=240;
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/fb0";
	LCD_HEIGHT = h;
	LCD_WIDTH = w;
	lcd_init(w, h, G_ROTATE_NONE);

	fbd_t fbd;
	fbd.splash = NULL;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	int ret = fbd_run(&fbd, mnt_point, LCD_WIDTH, LCD_HEIGHT, G_ROTATE_NONE);
	return ret;
}
