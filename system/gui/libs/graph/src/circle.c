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
	
	// Fill circle using distance-based algorithm (same as graph_circle but fill interior)
	// For each pixel in the bounding box, check if it's inside the circle
	for(int cy = -radius; cy <= radius; cy++) {
		for(int cx = -radius; cx <= radius; cx++) {
			int dist_sq = cx*cx + cy*cy;
			// Fill interior: distance <= radius
			if(dist_sq <= radius*radius) {
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
	
	// Draw circle outline with thickness rw
	// Similar to graph_round_3d but without 3D effect
	for(int cy = -radius; cy <= radius; cy++) {
		for(int cx = -radius; cx <= radius; cx++) {
			int dist_sq = cx*cx + cy*cy;
			// Draw arc with thickness rw
			if(dist_sq >= (radius-rw)*(radius-rw) && dist_sq <= radius*radius) {
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
