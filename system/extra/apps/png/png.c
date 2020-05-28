#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <upng/upng.h>

static void draw(graph_t* g, graph_t* img) {
	int x, y;
	clear(g, 0xff444444);
	for(y=10; y<(int32_t)g->h; y+=16) {
		for(x=0; x<(int32_t)g->w; x+=16) {
			fill(g, x, y, 6, 6, 0xff888888);
		}
	}

	blt_alpha(img, 0, 0, img->w, img->h,
			g, 0, 0, img->w, img->h, 0xff);
}

static void repaint(x_t* x, graph_t* g) {
	(void)x;
	graph_t* img = (graph_t*)x->data;
	draw(g, img);
}

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: png <png filename>\n");
		return -1;
	}

	printf("loading...");
	graph_t* img = png_image_new(argv[1]);
	if(img == NULL)  {
		printf("open '%s' error!\n", argv[1]);
		return -1;
	}
	printf("ok\n");

	x_t* x = x_open(10, 10, img->w, img->h+20, "png", X_STYLE_NORMAL | X_STYLE_NO_RESIZE);
	if(x == NULL) {
		graph_free(img);
		return -1;
	}
	x->data = img;
	x->on_repaint = repaint;
	x_set_visible(x, true);
	x_run(x);

	graph_free(img);
	x_close(x);
	return 0;
}

