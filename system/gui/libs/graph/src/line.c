#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif


static int32_t isqrt(int32_t x) {
	if(x <= 0) return 0;
	if(x == 1) return 1;
	
	int32_t res = x;
	int32_t tmp = (x + 1) / 2;
	while(tmp < res) {
		res = tmp;
		tmp = (tmp + x / tmp) / 2;
	}
	return res;
}

void graph_line(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2, uint32_t color) {
	if(g == NULL)
		return;

	if(x1 == x2 && y1 == y2) {
		if(color_a(color) == 0xff)
			graph_pixel(g, x1, y1, color);
		else
			graph_pixel_argb(g, x1, y1,
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
			graph_pixel(g, x, y, color);
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
			graph_pixel_argb(g, x, y, ca, cr, cg, cb);
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

	if(x1 == x2 && y1 == y2) {
		int32_t radius = w / 2;
		for(int32_t dy = -radius; dy <= radius; dy++) {
			for(int32_t dx = -radius; dx <= radius; dx++) {
				int32_t dist_sq = dx*dx + dy*dy;
				int32_t r_sq = radius*radius;
				if(dist_sq <= r_sq) {
					int32_t alpha = 0xff;
					if(dist_sq > r_sq - r_sq/4) {
						alpha = (r_sq - dist_sq) * 0xff / (r_sq/4);
					}
					graph_pixel_argb(g, x1 + dx, y1 + dy,
						alpha,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
				}
			}
		}
		return;
	}

	int32_t dx = x2 - x1;
	int32_t dy = y2 - y1;
	
	int32_t adx = dx < 0 ? -dx : dx;
	int32_t ady = dy < 0 ? -dy : dy;
	
	int32_t sx = dx < 0 ? -1 : 1;
	int32_t sy = dy < 0 ? -1 : 1;
	
	int32_t half_width = w / 2;
	
	int32_t len_sq = dx*dx + dy*dy;
	
	int32_t nx = -dy;
	int32_t ny = dx;
	int32_t n_len = isqrt(len_sq);
	
	/*int32_t start_ox = (nx * (-half_width - 1)) / n_len;
	int32_t start_oy = (ny * (-half_width - 1)) / n_len;
	int32_t end_ox = (nx * (half_width + 1)) / n_len;
	int32_t end_oy = (ny * (half_width + 1)) / n_len;
	
	graph_line(g, x1 + start_ox, y1 + start_oy, x1 + end_ox, y1 + end_oy,
		0x60 << 24 | ((color >> 16) & 0xff) << 16 | ((color >> 8) & 0xff) << 8 | (color & 0xff));
	graph_line(g, x2 + start_ox, y2 + start_oy, x2 + end_ox, y2 + end_oy,
		0x60 << 24 | ((color >> 16) & 0xff) << 16 | ((color >> 8) & 0xff) << 8 | (color & 0xff));
		*/
	
	if(adx >= ady) {
		int32_t e = 2 * ady - adx;
		int32_t x = x1;
		int32_t y = y1;
		
		for(int32_t i = 0; i <= adx; i++) {
			for(int32_t t = -half_width; t <= half_width; t++) {
				int32_t px = x;
				int32_t py = y + t;
				
				int32_t alpha = 0xff;
				if(t == -half_width || t == half_width) {
					alpha = 0x80;
				}
				
				graph_pixel_argb(g, px, py,
					alpha,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
			}
			if(e >= 0) {
				y += sy;
				e -= 2 * adx;
			}
			x += sx;
			e += 2 * ady;
		}
	} else {
		int32_t e = 2 * adx - ady;
		int32_t x = x1;
		int32_t y = y1;
		
		for(int32_t i = 0; i <= ady; i++) {
			for(int32_t t = -half_width; t <= half_width; t++) {
				int32_t px = x + t;
				int32_t py = y;
				
				int32_t alpha = 0xff;
				if(t == -half_width || t == half_width) {
					alpha = 0x80;
				}
				
				graph_pixel_argb(g, px, py,
					alpha,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
			}
			if(e >= 0) {
				x += sx;
				e -= 2 * ady;
			}
			y += sy;
			e += 2 * adx;
		}
	}
}

#ifdef __cplusplus
}
#endif
