#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif


void graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	if(g == NULL)
		return;

	if(x1 == x2 && y1 == y2) {
		if(color_a(color) == 0xff)
			graph_pixel_safe(g, x1, y1, color);
		else
			graph_pixel_argb_safe(g, x1, y1,
					(color >> 24) & 0xff,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
		return;
	}

	int32_t dx, dy, x, y, s1, s2, e, temp, swap, i;

	dy = y2 - y1;
	if (dy<0) {
		dy = -dy; 
		s2 = -1; 
	}
	else
		s2 = 1;

	dx = x2 - x1;
	if (dx<0) {
		dx = -dx; 
		s1 = -1;
	}
	else
		s1 = 1;

	x = x1;
	y = y1;
	if (dy > dx) {
		temp=dx;
		dx=dy;
		dy=temp;
		swap=1;
	}
	else
		swap=0;

	e = 2 * dy - dx;
	if(color_a(color) == 0xff) {
		for (i=0; i<=dx; i++) {
			graph_pixel_safe(g, x, y, color);
			while (e>=0) {
				if (swap==1)
					x += s1;
				else
					y += s2;
				e -= 2*dx;
			}
			if (swap==1)
				y += s2;
			else
				x += s1;
			e += 2*dy;
		}
	}
	else {
		register uint8_t ca = (color >> 24) & 0xff;
		register uint8_t cr = (color >> 16) & 0xff;
		register uint8_t cg = (color >> 8) & 0xff;
		register uint8_t cb = (color) & 0xff;
	
		for (i=0; i<=dx; i++) {
			graph_pixel_argb_safe(g, x, y, ca, cr, cg, cb);
			while (e>=0) {
				if (swap==1)
					x += s1;
				else 
					y += s2;
				e -= 2*dx;
			}
			if (swap==1)
				y += s2;
			else
				x += s1;
			e += 2*dy;
		}
	}
}

void graph_wline(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color, uint32_t w) {
	if(w == 0)
		return;

	if(x1 == x2)
		for(uint32_t i=0; i<w; i++)
			graph_line(g, x1+1, y1, x2+i, y2, color);
	else 
		for(uint32_t i=0; i<w; i++)
			graph_line(g, x1, y1+i, x2, y2+i, color);
}

#ifdef __cplusplus
}
#endif
