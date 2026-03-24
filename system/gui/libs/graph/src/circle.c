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
	
	int32_t r_plus_half = radius + 1;
	int32_t r_sq_plus = (r_plus_half * r_plus_half) - 1;
	
	for(int cy = -radius - 1; cy <= radius + 1; cy++) {
		for(int cx = -radius - 1; cx <= radius + 1; cx++) {
			int dist_sq = cx*cx + cy*cy;
			
			if(dist_sq <= r_sq_plus) {
				int px = x + cx;
				int py = y + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}
}

void graph_circle(graph_t* g, int32_t x, int32_t y, int32_t radius, int32_t rw, uint32_t color) {
	if(radius <= 0 || rw <= 0)
		return;
	if(rw > radius/2) rw = radius/2;
	
	int32_t outer_r = radius + 1;
	int32_t outer_r_sq = outer_r * outer_r - 1;
	
	int32_t inner_r = radius - rw;
	int32_t inner_r_adj = inner_r;
	int32_t inner_r_sq = inner_r_adj * inner_r_adj;
	
	for(int cy = -radius - 1; cy <= radius + 1; cy++) {
		for(int cx = -radius - 1; cx <= radius + 1; cx++) {
			int dist_sq = cx*cx + cy*cy;
			
			if(dist_sq >= inner_r_sq && dist_sq <= outer_r_sq) {
				int px = x + cx;
				int py = y + cy;
				if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
					graph_pixel_alpha(g, px, py, color);
				}
			}
		}
	}
}

#ifdef __cplusplus
}
#endif
