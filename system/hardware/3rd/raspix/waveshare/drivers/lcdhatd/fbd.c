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


static graph_t* _g = NULL;

static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	if(fbinfo->depth != 32)
		return -1;

	uint32_t sz = 4 * fbinfo->width * fbinfo->height;
	graph_t g;
	if(rotate == G_ROTATE_N90 || rotate == G_ROTATE_90) {
		graph_init(&g, buf, fbinfo->height, fbinfo->width);
		if(_g == NULL) {
			_g = graph_new(fbinfo->pointer, fbinfo->width, fbinfo->height);
		}
	}
	else if(rotate == G_ROTATE_180) {
		graph_init(&g, buf, fbinfo->width, fbinfo->height);
		if(_g == NULL) {
			_g = graph_new(fbinfo->pointer, fbinfo->width, fbinfo->height);
		}
	}

	if(_g != NULL) {
		graph_rotate_to(&g, _g, rotate);
		do_flush(_g->buffer, sz);
	}
	else  {
		do_flush(buf, sz);
	}
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
	_g = NULL;
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
	if(_g != NULL)
		graph_free(_g);
	return ret;
}
