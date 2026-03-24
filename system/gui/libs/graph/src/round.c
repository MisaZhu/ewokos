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

	// Draw four corners using filled arc algorithm
	// For each corner, we draw horizontal lines from the arc edge to the inner rectangle

	// Top-left corner
	for(int cy = 0; cy < r; cy++) {
		int min_cx = r;
		// Find the leftmost x for this y in the quarter circle
		for(int cx = 0; cx <= r; cx++) {
			int dx = cx - r;
			int dy = cy - r;
			if(dx*dx + dy*dy <= r*r) {
				min_cx = cx;
				break;
			}
		}
		// Draw horizontal line from arc edge to right edge of corner
		int py = y + cy;
		if(py >= 0 && py < g->h) {
			for(int px = x + min_cx; px <= x + r && px < g->w; px++) {
				if(px >= 0) graph_pixel_alpha_safe(g, px, py, color);
			}
		}
	}

	// Top-right corner
	for(int cy = 0; cy < r; cy++) {
		int max_cx = 0;
		// Find the rightmost x for this y in the quarter circle
		for(int cx = r-1; cx >= 0; cx--) {
			int dx = cx;
			int dy = cy - r;
			if(dx*dx + dy*dy <= r*r) {
				max_cx = cx;
				break;
			}
		}
		// Draw horizontal line from left edge of corner to arc edge
		int py = y + cy;
		if(py >= 0 && py < g->h) {
			for(int px = x + w - r; px <= x + w - r + max_cx && px < g->w; px++) {
				if(px >= 0) graph_pixel_alpha_safe(g, px, py, color);
			}
		}
	}

	// Bottom-left corner
	for(int cy = 0; cy < r; cy++) {
		int min_cx = r;
		// Find the leftmost x for this y in the quarter circle
		for(int cx = 0; cx <= r; cx++) {
			int dx = cx - r;
			int dy = cy;
			if(dx*dx + dy*dy <= r*r) {
				min_cx = cx;
				break;
			}
		}
		// Draw horizontal line from arc edge to right edge of corner
		int py = y + h - r + cy;
		if(py >= 0 && py < g->h) {
			for(int px = x + min_cx; px <= x + r && px < g->w; px++) {
				if(px >= 0) graph_pixel_alpha_safe(g, px, py, color);
			}
		}
	}

	// Bottom-right corner
	for(int cy = 0; cy < r; cy++) {
		int max_cx = 0;
		// Find the rightmost x for this y in the quarter circle
		for(int cx = r-1; cx >= 0; cx--) {
			int dx = cx;
			int dy = cy;
			if(dx*dx + dy*dy <= r*r) {
				max_cx = cx;
				break;
			}
		}
		// Draw horizontal line from left edge of corner to arc edge
		int py = y + h - r + cy;
		if(py >= 0 && py < g->h) {
			for(int px = x + w - r; px <= x + w - r + max_cx && px < g->w; px++) {
				if(px >= 0) graph_pixel_alpha_safe(g, px, py, color);
			}
		}
	}

	// Fill the remaining rectangular areas
	// Top bar (between corners)
	graph_fill(g, x+r+1, y, w-r*2-1, r, color);
	// Bottom bar (between corners)
	graph_fill(g, x+r+1, y+h-r, w-r*2-1, r, color);
	// Left bar (between corners)
	graph_fill(g, x, y+r, r, h-r*2, color);
	// Right bar (between corners)
	graph_fill(g, x+w-r, y+r, r, h-r*2, color);
}

void graph_round(graph_t* g, int32_t x, int32_t y, 
		int32_t w, int32_t h,
		int32_t r, uint32_t color) {
	if(r <= 1) {
		graph_box(g, x, y, w, h, color);
		return;
	}
	// Limit radius to half of width/height
	if(r > w/2) r = w/2;
	if(r > h/2) r = h/2;
	
	// Draw four corners using arc algorithm (consistent with graph_fill_round and graph_round_3d)
	// Top-left corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx - r;
			int dy = cy - r;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-1)*(r-1) && dist_sq <= r*r) {
				int px = x + cx;
				int py = y + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
	
	// Top-right corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy - r;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-1)*(r-1) && dist_sq <= r*r) {
				int px = x + w - r + cx;
				int py = y + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
	
	// Bottom-left corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx - r;
			int dy = cy;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-1)*(r-1) && dist_sq <= r*r) {
				int px = x + cx;
				int py = y + h - r + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
	
	// Bottom-right corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			int dx = cx;
			int dy = cy;
			int dist_sq = dx*dx + dy*dy;
			if(dist_sq >= (r-1)*(r-1) && dist_sq <= r*r) {
				int px = x + w - r + cx;
				int py = y + h - r + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha_safe(g, px, py, color);
				}
			}
		}
	}
	
	// Draw straight edges
	graph_line(g, x+r, y, x+w-r-1, y, color);
	graph_line(g, x+r, y+h-1, x+w-r-1, y+h-1, color);
	graph_line(g, x, y+r, x, y+h-r-1, color);
	graph_line(g, x+w-1, y+r, x+w-1, y+h-r-1, color);
}

#ifdef __cplusplus
}
#endif
