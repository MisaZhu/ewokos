#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <graph/tga.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		printf("Usage: tga <tga filename>\n");
		return -1;
	}

	printf("loading...");
	graph_t* img = tga_image_new(argv[1]);
	if(img == NULL)  {
		printf("open '%s' error!\n", argv[1]);
		return -1;
	}
	printf("ok.\n");

	x_t* x = x_open(100, 100, img->w, img->h+20, "tga", X_STYLE_NORMAL);
	if(x == NULL) {
		graph_free(img);
		return -1;
	}
	graph_t* g = x_get_graph(x);
	clear(g, 0xff888888);

	blt_alpha(img, 0, 0, img->w, img->h,
			g, 0, 0, img->w, img->h, 0xff);
	draw_text(g, 30, g->h-20, "press anykey to quit......", font_by_name("8x16"), 0xffffffff);
	x_release_graph(x, g);
	x_update(x);

	x_set_visible(x, true);

	xevent_t xev;
	while(x->closed == 0) {
		if(x_get_event(x, &xev) == 0) {
			if(xev.type == XEVT_KEYB)
				break;
		}
		sleep(0);
	}

	graph_free(img);
	x_close(x);
	return 0;
}

