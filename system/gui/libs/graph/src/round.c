#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Helper function: draw anti-aliased pixel (using integer alpha)
static inline void draw_aa_pixel_int(graph_t* g, int32_t x, int32_t y, uint32_t color, uint8_t alpha) {
    if (alpha <= 0) return;

    if (alpha >= 255) {
        graph_pixel(g, x, y, (color & 0x00FFFFFF) | 0xFF000000);
    } else {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t gc = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        graph_pixel_argb(g, x, y, alpha, r, gc, b);
    }
}

// Estimate disk coverage using 4x4 subpixel sampling over the pixel square.
// The local pixel region is [px, px+1] x [py, py+1], where px/py are distances
// measured from the rounded corner's circle center toward the outer edges.
static inline float round_disk_coverage(float px, float py, float radius) {
    static const float sample_offsets[4] = {0.125f, 0.375f, 0.625f, 0.875f};
    float radius_sq = radius * radius;
    int covered = 0;

    for (int sy = 0; sy < 4; sy++) {
        float y = py + sample_offsets[sy];
        for (int sx = 0; sx < 4; sx++) {
            float x = px + sample_offsets[sx];
            if ((x * x + y * y) <= radius_sq)
                covered++;
        }
    }

    return (float)covered / 16.0f;
}

// Helper function: draw rounded corner using quarter-circle coverage.
static inline void draw_round_corner_fill(graph_t* g, int32_t corner_x, int32_t corner_y,
                                          int32_t cx, int32_t cy, int32_t r,
                                          uint32_t color, int mirror_x, int mirror_y) {
    uint8_t fg_alpha = (color >> 24) & 0xFF;

    for (int32_t dy = 0; dy < r; dy++) {
        for (int32_t dx = 0; dx < r; dx++) {
            int px = corner_x + cx + (mirror_x ? -dx : dx);
            int py = corner_y + cy + (mirror_y ? -dy : dy);

            if (px < 0 || px >= g->w || py < 0 || py >= g->h)
                continue;

            float cov = round_disk_coverage((float)dx, (float)dy, (float)r);
            uint8_t alpha = (uint8_t)(fg_alpha * cov);

            if (alpha > 0)
                draw_aa_pixel_int(g, px, py, color, alpha);
        }
    }
}

// Helper function: draw rounded corner border using quarter-circle ring coverage.
static inline void draw_round_corner_round(graph_t* g, int32_t corner_x, int32_t corner_y,
                                           int32_t cx, int32_t cy, int32_t r, int32_t rw,
                                           uint32_t color, int mirror_x, int mirror_y) {
    uint8_t fg_alpha = (color >> 24) & 0xFF;

    float outer_radius = (float)r;
    float inner_radius = (float)(r - rw);
    if (rw == 1)
        inner_radius -= 0.5f;
    if (inner_radius < 0.0f)
        inner_radius = 0.0f;

    for (int32_t dy = 0; dy < r; dy++) {
        for (int32_t dx = 0; dx < r; dx++) {
            int px = corner_x + cx + (mirror_x ? -dx : dx);
            int py = corner_y + cy + (mirror_y ? -dy : dy);

            if (px < 0 || px >= g->w || py < 0 || py >= g->h)
                continue;

            float outer_cov = round_disk_coverage((float)dx, (float)dy, outer_radius);
            float inner_cov = 0.0f;
            if (inner_radius > 0.0f)
                inner_cov = round_disk_coverage((float)dx, (float)dy, inner_radius);

            float cov = outer_cov - inner_cov;
            if (cov < 0.0f)
                cov = 0.0f;

            uint8_t alpha = (uint8_t)(fg_alpha * cov);
            if (alpha > 0)
                draw_aa_pixel_int(g, px, py, color, alpha);
        }
    }
}

void graph_fill_round(graph_t* g, int32_t x, int32_t y,
		int32_t w, int32_t h,
		int32_t r, uint32_t color) {
	if(r <= 1) {
		graph_fill_rect(g, x, y, w, h, color);
		return;
	}
	// Limit radius to half of width/height
	if(r > w/2) r = w/2;
	if(r > h/2) r = h/2;

	// Draw main rectangle body (non-corner area)
	graph_fill_rect(g, x+r, y+r, w-r*2, h-r*2, color);

	// Fill the remaining rectangular areas
	// Top bar (between corners)
	graph_fill_rect(g, x+r, y, w-r*2, r, color);
	// Bottom bar (between corners)
	graph_fill_rect(g, x+r, y+h-r, w-r*2, r, color);
	// Left bar (between corners)
	graph_fill_rect(g, x, y+r, r, h-r*2, color);
	// Right bar (between corners)
	graph_fill_rect(g, x+w-r, y+r, r, h-r*2, color);

	// Draw four corners with anti-aliasing (scanline optimization, non-floating point)
	// Top-left corner
	draw_round_corner_fill(g, x, y, r-1, r-1, r, color, 1, 1);

	// Top-right corner
	draw_round_corner_fill(g, x+w-r, y, 0, r-1, r, color, 0, 1);

	// Bottom-left corner
	draw_round_corner_fill(g, x, y+h-r, r-1, 0, r, color, 1, 0);

	// Bottom-right corner
	draw_round_corner_fill(g, x+w-r, y+h-r, 0, 0, r, color, 0, 0);
}

void graph_round(graph_t* g, int32_t x, int32_t y,
		int32_t w, int32_t h,
		int32_t r, int32_t rw, uint32_t color) {
	if(r <= 1) {
		graph_rect(g, x, y, w, h, color);
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
				graph_pixel(g, xx, yy, color);
			}
		}
	}
	for(int i = 0; i < rw; i++) {
		int yy = y + h - 1 - i;
		if(yy >= 0 && yy < g->h) {
			for(int xx = x + r; xx < x + w - r; xx++) {
				graph_pixel(g, xx, yy, color);
			}
		}
	}
	for(int i = 0; i < rw; i++) {
		int xx = x + i;
		if(xx >= 0 && xx < g->w) {
			for(int yy = y + r; yy < y + h - r; yy++) {
				graph_pixel(g, xx, yy, color);
			}
		}
	}
	for(int i = 0; i < rw; i++) {
		int xx = x + w - 1 - i;
		if(xx >= 0 && xx < g->w) {
			for(int yy = y + r; yy < y + h - r; yy++) {
				graph_pixel(g, xx, yy, color);
			}
		}
	}

	// Draw four corners with anti-aliasing (scanline optimization, non-floating point)
	// Top-left corner
	draw_round_corner_round(g, x, y, r-1, r-1, r, rw, color, 1, 1);

	// Top-right corner
	draw_round_corner_round(g, x+w-r, y, 0, r-1, r, rw, color, 0, 1);

	// Bottom-left corner
	draw_round_corner_round(g, x, y+h-r, r-1, 0, r, rw, color, 1, 0);

	// Bottom-right corner
	draw_round_corner_round(g, x+w-r, y+h-r, 0, 0, r, rw, color, 0, 0);
}

void graph_semi_round(graph_t* g, int32_t x, int32_t y,
		int32_t w, int32_t r, int32_t rw, uint32_t color, bool top_half) {
	if(r <= 1) {
		if(top_half) {
			for(int i = 0; i < rw; i++) {
				int yy = y + i;
				if(yy >= 0 && yy < g->h) {
					for(int xx = x; xx < x + w; xx++) {
						graph_pixel(g, xx, yy, color);
					}
				}
			}
		} else {
			for(int i = 0; i < rw; i++) {
				int yy = y + r - 1 - i;
				if(yy >= 0 && yy < g->h) {
					for(int xx = x; xx < x + w; xx++) {
						graph_pixel(g, xx, yy, color);
					}
				}
			}
		}
		return;
	}
	// Limit radius to half of width
	if(r > w/2) r = w/2;
	if(rw > r/2) rw = r/2;

	if(top_half) {
		// Top half: draw top edge
		// Draw top edge
		for(int i = 0; i < rw; i++) {
			int yy = y + i;
			if(yy >= 0 && yy < g->h) {
				for(int xx = x + r; xx < x + w - r; xx++) {
					graph_pixel(g, xx, yy, color);
				}
			}
		}

		// Draw two top corners with anti-aliasing
		// Top-left corner
		draw_round_corner_round(g, x, y, r-1, r-1, r, rw, color, 1, 1);

		// Top-right corner
		draw_round_corner_round(g, x+w-r, y, 0, r-1, r, rw, color, 0, 1);
	} else {
		// Bottom half: draw bottom edge
		// Draw bottom edge
		for(int i = 0; i < rw; i++) {
			int yy = y + r - 1 - i;
			if(yy >= 0 && yy < g->h) {
				for(int xx = x + r; xx < x + w - r; xx++) {
					graph_pixel(g, xx, yy, color);
				}
			}
		}

		// Draw two bottom corners with anti-aliasing
		// Bottom-left corner
		draw_round_corner_round(g, x, y, r-1, 0, r, rw, color, 1, 0);

		// Bottom-right corner
		draw_round_corner_round(g, x+w-r, y, 0, 0, r, rw, color, 0, 0);
	}
}

void graph_semi_fill_round(graph_t* g, int32_t x, int32_t y,
		int32_t w, int32_t r, uint32_t color, bool top_half) {
	if(r <= 1) {
		graph_fill_rect(g, x, y, w, r, color);
		return;
	}
	// Limit radius to half of width
	if(r > w/2) r = w/2;

	if(top_half) {
		// Top half
		// Fill top bar (between corners)
		graph_fill_rect(g, x+r, y, w-r*2, r, color);

		// Draw two top corners with anti-aliasing
		// Top-left corner
		draw_round_corner_fill(g, x, y, r-1, r-1, r, color, 1, 1);

		// Top-right corner
		draw_round_corner_fill(g, x+w-r, y, 0, r-1, r, color, 0, 1);
	} else {
		// Bottom half
		// Fill bottom bar (between corners)
		graph_fill_rect(g, x+r, y, w-r*2, r, color);

		// Draw two bottom corners with anti-aliasing
		// Bottom-left corner
		draw_round_corner_fill(g, x, y, r-1, 0, r, color, 1, 0);

		// Bottom-right corner
		draw_round_corner_fill(g, x+w-r, y, 0, 0, r, color, 0, 0);
	}
}

#ifdef __cplusplus
}
#endif
