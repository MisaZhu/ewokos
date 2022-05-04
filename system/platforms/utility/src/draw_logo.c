#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <graph/graph.h>
#include <fonts/fonts.h>
#include <upng/upng.h>

void draw_logo(graph_t* g) {
	if(g == NULL)
		return;

	const char* welcome = "Ewok Micro Kernel OS(Pi)";
	gsize_t sz;
	font_t* font = font_by_name("10x20");
	get_text_size(welcome, font, &sz.w, &sz.h);

	int x = (g->w - sz.w) / 2;
	int y = (g->h - sz.h) / 2;
	int m = 12;

	graph_clear(g, 0xff000000);
	int32_t i, j;
	for(j=10; j<(int32_t)g->h; j+=10) {
		for(i=0; i<(int32_t)g->w; i+=10) {
			graph_pixel(g, i, j, 0xff444444);
		}
	}

	graph_fill_round(g,
		x - m, y - m,
		sz.w + 2*m, sz.h + 2*m, 
		8, 0x88ffffff);

	m = 16;
	graph_round(g,
		x - m, y - m,
		sz.w + 2*m, sz.h + 2*m, 
		12, 0x88ffffff);

	graph_draw_text(g,
		x, y,
		welcome,  font, 0xff000000);

	graph_t* img = png_image_new("/data/icons/starwars/yoda.png");
	if(img != NULL) {
		m = img->w;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x-m, y-img->h/2, img->w, img->h, 0xff);
		graph_free(img);
	}
}