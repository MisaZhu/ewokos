#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <fbd/fbd.h>
#include <graph/graph.h>

static uint32_t LCD_HEIGHT;
static uint32_t LCD_WIDTH;

int  do_flush(const void* buf, uint32_t size);
static uint32_t flush(const fbinfo_t* fbinfo, const void* buf, uint32_t size, int rotate) {
	(void)rotate;
	graph_t g;
	graph_init(&g, buf, LCD_WIDTH, LCD_HEIGHT);
	graph_t* fbg = graph_rotate(&g, G_ROTATE_90);
	int ret = do_flush(fbg->buffer, size);
	graph_free(fbg);
	return ret;
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
	epaper_init();
	return 0;
}

int main(int argc, char** argv) {
	LCD_HEIGHT = 104;
	LCD_WIDTH = 212;

	const char* mnt_point = argc > 1 ? argv[1]: "/dev/epaper";

	fbd_t fbd;
	fbd.flush = flush;
	fbd.init = init;
	fbd.get_info = get_info;
	return fbd_run(&fbd, mnt_point, LCD_WIDTH, LCD_HEIGHT, G_ROTATE_NONE);
}
