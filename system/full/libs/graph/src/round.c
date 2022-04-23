#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif


void graph_fill_round(graph_t* g, int32_t x, int32_t y, 
		int32_t w, int32_t h,
		int32_t radius, uint32_t color) {
	if(radius <= 1) {
		graph_fill(g, x, y, w, h, color);
		return;
	}

	int32_t a, b, P, xr, yr;

	//left top round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + radius;
	yr = y + radius;

	//graph_line(g, xr-radius, yr, xr, yr, color);
	do {
		graph_line(g, xr-b, yr-a, xr, yr-a, color);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			if(a != b) {
				graph_line(g, xr-a, yr-b, xr, yr-b, color);
			}
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//left down round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + radius;
	yr = y + h - radius-1;

	//graph_line(g, xr-radius, yr, xr, yr, color);
	do {
		graph_line(g, xr-b, yr+a, xr, yr+a, color);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			if(a != b) {
				graph_line(g, xr-a, yr+b, xr, yr+b, color);
			}
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//right top round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + w - radius - 1;
	yr = y + radius;

	//graph_line(g, xr, yr, xr+radius, yr, color);
	do {
		graph_line(g, xr, yr-a, xr+b, yr-a, color);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			if(a != b) {
				graph_line(g, xr, yr-b, xr+a, yr-b, color);
			}
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//right down round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + w - radius-1;
	yr = y + h - radius-1;

	//graph_line(g, xr, yr, xr+radius, yr, color);
	do {
		graph_line(g, xr, yr+a, xr+b, yr+a, color);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			if(a != b) {
				graph_line(g, xr, yr+b, xr+a, yr+b, color);
			}
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//top bar
	graph_fill(g, x+radius+1, y, w-radius*2-2, radius, color);
	graph_fill(g, x+radius+1, y+h-radius, w-radius*2-2, radius, color);
	graph_fill(g, x, y+radius, w, h-radius*2, color);
}

void graph_round(graph_t* g, int32_t x, int32_t y, 
		int32_t w, int32_t h,
		int32_t radius, uint32_t color) {
	if(radius <= 1) {
		graph_box(g, x, y, w, h, color);
		return;
	}

	int32_t a, b, P, xr, yr;

	uint8_t ca = (color >> 24) & 0xff;
	uint8_t cr = (color >> 16) & 0xff;
	uint8_t cg = (color >> 8) & 0xff;
	uint8_t cb = (color) & 0xff;

	//left top round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + radius;
	yr = y + radius;

	do {
		graph_pixel_argb_safe(g, xr-b, yr-a, ca, cr, cg, cb);
				graph_pixel_argb_safe(g, xr-a, yr-b, ca, cr, cg, cb);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//left down round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + radius;
	yr = y + h - radius-1;

	do {
		graph_pixel_argb_safe(g, xr-b, yr+a, ca, cr, cg, cb);
				graph_pixel_argb_safe(g, xr-a, yr+b, ca, cr, cg, cb);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//right top round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + w - radius - 1;
	yr = y + radius;

	do {
		graph_pixel_argb_safe(g, xr+b, yr-a, ca, cr, cg, cb);
				graph_pixel_argb_safe(g, xr+a, yr-b, ca, cr, cg, cb);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	//right down round
	a = 1;
	b = radius;
	P = 1 - radius;
	xr = x + w - radius-1;
	yr = y + h - radius-1;

	do {
		graph_pixel_argb_safe(g, xr+b, yr+a, ca, cr, cg, cb);
				graph_pixel_argb_safe(g, xr+a, yr+b, ca, cr, cg, cb);
		if(P < 0) {
			P += 3 + 2*a++;
		}
		else {
			P += 5 + 2*(a++ - b--);
		}
	} while(a <= b);

	graph_line(g, x+radius, y, x+w-radius-1, y, color);
	graph_line(g, x+radius, y+h-1, x+w-radius-1, y+h-1, color);
	graph_line(g, x, y+radius, x, y+h-radius-1, color);
	graph_line(g, x+w-1, y+radius, x+w-1, y+h-radius-1, color);
}

#ifdef __cplusplus
}
#endif
