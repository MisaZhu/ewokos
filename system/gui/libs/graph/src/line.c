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
					alpha = (alpha * ((color >> 24) & 0xff)) / 0xff;
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
	
	int32_t half_width = w / 2;
	int32_t color_alpha = (color >> 24) & 0xff;
	
	int32_t len_sq = dx*dx + dy*dy;
	int32_t len = isqrt(len_sq);
	
	int32_t min_x = (x1 < x2 ? x1 : x2) - half_width - 2;
	int32_t max_x = (x1 > x2 ? x1 : x2) + half_width + 2;
	int32_t min_y = (y1 < y2 ? y1 : y2) - half_width - 2;
	int32_t max_y = (y1 > y2 ? y1 : y2) + half_width + 2;
	
	for(int32_t y = min_y; y <= max_y; y++) {
		for(int32_t x = min_x; x <= max_x; x++) {
			int32_t coverage = 0;
			
			for(int32_t sy = 0; sy < 2; sy++) {
				for(int32_t sx = 0; sx < 2; sx++) {
					int32_t sub_x = x * 2 + sx;
					int32_t sub_y = y * 2 + sy;
					
					int32_t px = sub_x - x1 * 2;
					int32_t py = sub_y - y1 * 2;
					
					int32_t dot = px * dx + py * dy;
					int32_t t;
					
					if(dot < 0) {
						t = 0;
					} else if(dot > len_sq * 4) {
						t = len_sq * 4;
					} else {
						t = dot;
					}
					
					int32_t closest_x = x1 * 2 + (dx * t) / len_sq;
					int32_t closest_y = y1 * 2 + (dy * t) / len_sq;
					
					int32_t dist_sq = (sub_x - closest_x) * (sub_x - closest_x) + (sub_y - closest_y) * (sub_y - closest_y);
					int32_t hw_sq = half_width * half_width * 4;
					
					if(dist_sq <= hw_sq) {
						coverage++;
					} else if(dist_sq <= hw_sq + half_width * 2 + 1) {
						coverage++;
					}
				}
			}
			
			if(coverage > 0) {
				int32_t alpha = (coverage * 0xff) / 4;
				alpha = (alpha * color_alpha) / 0xff;
				
				graph_pixel_argb(g, x, y,
					alpha,
					(color >> 16) & 0xff,
					(color >> 8) & 0xff,
					color & 0xff);
			}
		}
	}
}

#ifdef __cplusplus
}
#endif
