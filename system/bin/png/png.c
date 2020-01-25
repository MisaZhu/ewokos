#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <upng/upng.h>

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

	x_t* x = x_open(10, 10, img->w, img->h+20, "png", X_STYLE_NORMAL);
	if(x == NULL) {
		graph_free(img);
		return -1;
	}
	graph_t* g = x_get_graph(x);
	clear(g, 0xffffffff);

	blt_alpha(img, 0, 0, img->w, img->h,
			g, 0, 0, img->w, img->h, 0xff);
	x_release_graph(x, g);
	x_update(x);

	x_set_visible(x, true);
	x_run(x, NULL, NULL, NULL);

	graph_free(img);
	x_close(x);
	return 0;
}

