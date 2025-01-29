#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

void graph_fill_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color) {
	if(radius <= 0)
		return;

	int32_t a, b, P;
	a = 1;
	b = radius;
	P = 1 - radius;

	graph_line(g, x-radius, y, x+radius, y, color);
	do {
		graph_line(g, x-b, y+a, x+b, y+a, color);
		graph_line(g, x-b, y-a, x+b, y-a, color);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			if(a != b) {
				graph_line(g, x - a, y + b, x + a, y + b, color);
				graph_line(g, x - a, y - b, x + a, y - b, color);
			}
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);
}

void graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, uint32_t color) {
	if(radius <= 0)
		return;
	int32_t a, b, P;
	a = 1;
	b = radius;
	P = 1 - radius;

	if(color_a(color) != 0xff) {
		uint8_t ca = (color >> 24) & 0xff;
		uint8_t cr = (color >> 16) & 0xff;
		uint8_t cg = (color >> 8) & 0xff;
		uint8_t cb = (color) & 0xff;

		graph_pixel_argb_safe(g, x, radius+y, ca, cr, cg, cb);
		graph_pixel_argb_safe(g, x, y-radius, ca, cr, cg, cb);
		graph_pixel_argb_safe(g, x+radius, y, ca, cr, cg, cb);
		graph_pixel_argb_safe(g, x-radius, y, ca, cr, cg, cb);

		do {
			graph_pixel_argb_safe(g, b+x, a+y, ca, cr, cg, cb);
			graph_pixel_argb_safe(g, x-b, a+y, ca, cr, cg, cb);
			graph_pixel_argb_safe(g, b+x, y-a, ca, cr, cg, cb);
			graph_pixel_argb_safe(g, x-b, y-a, ca, cr, cg, cb);

			graph_pixel_argb_safe(g, a+x, b+y, ca, cr, cg, cb);
			graph_pixel_argb_safe(g, x-a, b+y, ca, cr, cg, cb);
			graph_pixel_argb_safe(g, a+x, y-b, ca, cr, cg, cb);
			graph_pixel_argb_safe(g, x-a, y-b, ca, cr, cg, cb);

			if(P < 0)
				P+= 3 + 2*a++;
			else
				P+= 5 + 2*(a++ - b--);
		} while(a < b);
		graph_pixel_argb_safe(g, b+x, a+y, ca, cr, cg, cb);
		graph_pixel_argb_safe(g, x-b, a+y, ca, cr, cg, cb);
		graph_pixel_argb_safe(g, b+x, y-a, ca, cr, cg, cb);
		graph_pixel_argb_safe(g, x-b, y-a, ca, cr, cg, cb);
	}
	else {
		graph_pixel_safe(g, x, radius+y, color);
		graph_pixel_safe(g, x, y-radius, color);
		graph_pixel_safe(g, x+radius, y, color);
		graph_pixel_safe(g, x-radius, y, color);

		do {
			graph_pixel_safe(g, b+x, a+y, color);
			graph_pixel_safe(g, x-b, a+y, color);
			graph_pixel_safe(g, b+x, y-a, color);
			graph_pixel_safe(g, x-b, y-a, color);

			graph_pixel_safe(g, a+x, b+y, color);
			graph_pixel_safe(g, x-a, b+y, color);
			graph_pixel_safe(g, a+x, y-b, color);
			graph_pixel_safe(g, x-a, y-b, color);

			if(P < 0)
				P+= 3 + 2*a++;
			else
				P+= 5 + 2*(a++ - b--);
		} while(a < b);
		graph_pixel_safe(g, b+x, a+y, color);
		graph_pixel_safe(g, x-b, a+y, color);
		graph_pixel_safe(g, b+x, y-a, color);
		graph_pixel_safe(g, x-b, y-a, color);
	}
}

#ifdef __cplusplus
}
#endif
