#include <types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <kstring.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <graph/tga.h>

int main(int argc, char* argv[]) {
	if(argc < 2)
		return -1;

	printf("loading...");
	graph_t* img = tga_image_new(argv[1]);
	if(img == NULL)  {
		printf("error!\n");
		return -1;
	}
	printf("ok.\n");

	const char* dev_name = "/dev/X0";
	x_t x;
	if(x_open(dev_name, &x) != 0) {
		graph_free(img);
		return -1;
	}
	
	x_set_title(&x, argv[0]);
	x_move_to(&x, 100, 100);
	x_resize_to(&x, img->w, img->h);
	x_set_state(&x, X_STATE_NORMAL);
	graph_t* g = x_get_graph(&x);
	clear(g, 0x0);
	blt_alpha(img, 0, 0, img->w, img->h,
			g, 0, 0, img->w, img->h, 0xff);
	x_flush(&x);

	while(true) {
		x_ev_t ev;
		if(x_get_event(dev_name, &ev) == 0) {
			if(ev.type == X_EV_WIN) {
				if(ev.state == X_EV_WIN_CLOSE)
					break;
			}
		}
	}

	graph_free(img);
	graph_free(g);
	x_close(&x);
	return 0;
}

