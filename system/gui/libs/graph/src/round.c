#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif


void graph_fill_round(graph_t* g, int32_t x, int32_t y,
		int32_t w, int32_t h,
		int32_t r, uint32_t color) {
	if(r <= 1) {
		graph_fill(g, x, y, w, h, color);
		return;
	}
	// Limit radius to half of width/height
	if(r > w/2) r = w/2;
	if(r > h/2) r = h/2;

	// Draw main rectangle body
	graph_fill(g, x+r, y+r, w-r*2, h-r*2, color);

	// Draw four corners using filled arc algorithm with consistent distance calculation
	// All corners use dx=cx, dy=cy like bottom-right for consistent rounding

	// Top-left corner - mirror position from bottom-right pattern
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			if(dx*dx + dy*dy <= r*r) {
				int px = x + (r - 1 - cx);
				int py = y + (r - 1 - cy);
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}

	// Top-right corner - mirror vertically from bottom-right pattern
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			if(dx*dx + dy*dy <= r*r) {
				int px = x + w - r + cx;
				int py = y + (r - 1 - cy);
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}

	// Bottom-left corner - mirror horizontally from bottom-right pattern
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			if(dx*dx + dy*dy <= r*r) {
				int px = x + (r - 1 - cx);
				int py = y + h - r + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}

	// Bottom-right corner (original pattern)
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			if(dx*dx + dy*dy <= r*r) {
				int px = x + w - r + cx;
				int py = y + h - r + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}

	// Fill the remaining rectangular areas
	// Top bar (between corners)
	graph_fill(g, x+r, y, w-r*2, r, color);
	// Bottom bar (between corners)
	graph_fill(g, x+r, y+h-r, w-r*2, r, color);
	// Left bar (between corners)
	graph_fill(g, x, y+r, r, h-r*2, color);
	// Right bar (between corners)
	graph_fill(g, x+w-r, y+r, r, h-r*2, color);
}

void graph_round(graph_t* g, int32_t x, int32_t y, 
		int32_t w, int32_t h,
		int32_t r, int32_t rw, uint32_t color) {
	if(r <= 1) {
		graph_box(g, x, y, w, h, color);
		return;
	}
	// Limit radius to half of width/height
	if(r > w/2) r = w/2;
	if(r > h/2) r = h/2;
	if(rw > r/2) rw = r/2;
	
	// Draw straight edges
	for(int i = 0; i < rw; i++) {
		int yy = y + i;
		if(yy >= 0 && yy < g->h) {
			for(int xx = x + r; xx < x + w - r; xx++) {
				if(xx >= 0 && xx < g->w) {
					graph_pixel_alpha(g, xx, yy, color);
				}
			}
		}
	}
	for(int i = 0; i < rw; i++) {
		int yy = y + h - 1 - i;
		if(yy >= 0 && yy < g->h) {
			for(int xx = x + r; xx < x + w - r; xx++) {
				if(xx >= 0 && xx < g->w) {
					graph_pixel_alpha(g, xx, yy, color);
				}
			}
		}
	}
	for(int i = 0; i < rw; i++) {
		int xx = x + i;
		if(xx >= 0 && xx < g->w) {
			for(int yy = y + r; yy < y + h - r; yy++) {
				if(yy >= 0 && yy < g->h) {
					graph_pixel_alpha(g, xx, yy, color);
				}
			}
		}
	}
	for(int i = 0; i < rw; i++) {
		int xx = x + w - 1 - i;
		if(xx >= 0 && xx < g->w) {
			for(int yy = y + r; yy < y + h - r; yy++) {
				if(yy >= 0 && yy < g->h) {
					graph_pixel_alpha(g, xx, yy, color);
				}
			}
		}
	}
	
	// Draw four corners using arc algorithm with consistent distance calculation
	// All corners use dx=cx, dy=cy like bottom-right for consistent rounding
	
	// Top-left corner - mirror position from bottom-right pattern
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
				int px = x + (r - 1 - cx);
				int py = y + (r - 1 - cy);
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}
	
	// Top-right corner - mirror vertically from bottom-right pattern
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
				int px = x + w - r + cx;
				int py = y + (r - 1 - cy);
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
	
	// Bottom-left corner - mirror horizontally from bottom-right pattern
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
				int px = x + (r - 1 - cx);
				int py = y + h - r + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
	
	// Bottom-right corner (original pattern)
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
				int px = x + w - r + cx;
				int py = y + h - r + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
}

#ifdef __cplusplus
}
#endif
