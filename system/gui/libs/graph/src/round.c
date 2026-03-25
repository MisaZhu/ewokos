#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef BSP_BOOST

// 辅助函数：混合两个颜色（与graph_pixel_argb_raw一致的混合方式）
// alpha是前景色的强度(0.0-1.0)，fg是前景色，bg是背景色
static inline uint32_t blend_colors(uint32_t bg, uint32_t fg, float alpha) {
    if (alpha <= 0.0f) return bg;
    if (alpha >= 1.0f) return fg;
    
    // 将float alpha转换为0-255范围的整数，与graph_pixel_argb_raw一致
    uint8_t a = (uint8_t)(alpha * 255.0f);
    
    uint8_t bg_r = (bg >> 16) & 0xFF;
    uint8_t bg_g = (bg >> 8) & 0xFF;
    uint8_t bg_b = bg & 0xFF;
    
    uint8_t fg_r = (fg >> 16) & 0xFF;
    uint8_t fg_g = (fg >> 8) & 0xFF;
    uint8_t fg_b = fg & 0xFF;
    
    // 使用与graph_pixel_argb_raw相同的混合公式
    uint8_t out_r = ((fg_r * a) >> 8) + ((bg_r * (255 - a)) >> 8);
    uint8_t out_g = ((fg_g * a) >> 8) + ((bg_g * (255 - a)) >> 8);
    uint8_t out_b = ((fg_b * a) >> 8) + ((bg_b * (255 - a)) >> 8);
    
    // alpha通道：结果alpha = 255（不透明），因为我们是在绘制到已存在的背景上
    return ((uint32_t)0xFF << 24) | ((uint32_t)out_r << 16) | ((uint32_t)out_g << 8) | out_b;
}

// 辅助函数：计算抗锯齿强度（基于到理想边界的距离）
// 距离为0表示完全在内部，距离为1表示完全在外部
static inline float aa_intensity(float dist) {
    if (dist <= -0.5f) return 1.0f;
    if (dist >= 0.5f) return 0.0f;
    return 0.5f - dist;
}

// 辅助函数：绘制抗锯齿像素
static inline void draw_aa_pixel(graph_t* g, int32_t x, int32_t y, uint32_t color, float intensity) {
    if (intensity <= 0.0f) return;
    
    uint8_t fg_alpha = (color >> 24) & 0xFF;
    // 最终alpha = 颜色本身的alpha * 抗锯齿强度
    uint8_t final_alpha = (uint8_t)((fg_alpha * intensity) + 0.5f);
    
    if (final_alpha >= 0xFF) {
        // 完全不透明，直接绘制
        graph_pixel(g, x, y, (color & 0x00FFFFFF) | 0xFF000000);
    } else if (final_alpha > 0) {
        // 需要alpha混合
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t gc = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        graph_pixel_argb(g, x, y, final_alpha, r, gc, b);
    }
}

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

	float r_f = (float)r;

	// Draw main rectangle body (non-corner area)
	graph_fill(g, x+r, y+r, w-r*2, h-r*2, color);

	// Fill the remaining rectangular areas
	// Top bar (between corners)
	graph_fill(g, x+r, y, w-r*2, r, color);
	// Bottom bar (between corners)
	graph_fill(g, x+r, y+h-r, w-r*2, r, color);
	// Left bar (between corners)
	graph_fill(g, x, y+r, r, h-r*2, color);
	// Right bar (between corners)
	graph_fill(g, x+w-r, y+r, r, h-r*2, color);

	// Draw four corners with anti-aliasing
	// Top-left corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			float edge_dist = dist - r_f;
			float intensity = aa_intensity(edge_dist);
			
			int px = x + (r - 1 - cx);
			int py = y + (r - 1 - cy);
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}

	// Top-right corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			float edge_dist = dist - r_f;
			float intensity = aa_intensity(edge_dist);
			
			int px = x + w - r + cx;
			int py = y + (r - 1 - cy);
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}

	// Bottom-left corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			float edge_dist = dist - r_f;
			float intensity = aa_intensity(edge_dist);
			
			int px = x + (r - 1 - cx);
			int py = y + h - r + cy;
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}

	// Bottom-right corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			float edge_dist = dist - r_f;
			float intensity = aa_intensity(edge_dist);
			
			int px = x + w - r + cx;
			int py = y + h - r + cy;
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}
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
	
	float r_f = (float)r;
	float inner_r_f = (float)(r - rw);
	
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
	
	// Draw four corners with anti-aliasing
	// Top-left corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			
			// 计算到内外边界的距离
			float outer_edge = dist - r_f;
			float inner_edge = inner_r_f - dist;
			
			// 计算抗锯齿强度
			float intensity = 0.0f;
			
			if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
				// 在外边缘
				intensity = aa_intensity(outer_edge);
			} else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
				// 在内边缘
				intensity = aa_intensity(inner_edge);
			} else if (outer_edge < 0.0f && inner_edge < 0.0f) {
				// 在圆环内部
				intensity = 1.0f;
			}
			
			int px = x + (r - 1 - cx);
			int py = y + (r - 1 - cy);
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}
	
	// Top-right corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			
			float outer_edge = dist - r_f;
			float inner_edge = inner_r_f - dist;
			
			float intensity = 0.0f;
			
			if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
				intensity = aa_intensity(outer_edge);
			} else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
				intensity = aa_intensity(inner_edge);
			} else if (outer_edge < 0.0f && inner_edge < 0.0f) {
				intensity = 1.0f;
			}
			
			int px = x + w - r + cx;
			int py = y + (r - 1 - cy);
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}
	
	// Bottom-left corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			
			float outer_edge = dist - r_f;
			float inner_edge = inner_r_f - dist;
			
			float intensity = 0.0f;
			
			if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
				intensity = aa_intensity(outer_edge);
			} else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
				intensity = aa_intensity(inner_edge);
			} else if (outer_edge < 0.0f && inner_edge < 0.0f) {
				intensity = 1.0f;
			}
			
			int px = x + (r - 1 - cx);
			int py = y + h - r + cy;
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}
	
	// Bottom-right corner
	for(int cy = 0; cy < r; cy++) {
		for(int cx = 0; cx < r; cx++) {
			float dx = (float)cx + 0.5f;
			float dy = (float)cy + 0.5f;
			float dist = sqrtf(dx * dx + dy * dy);
			
			float outer_edge = dist - r_f;
			float inner_edge = inner_r_f - dist;
			
			float intensity = 0.0f;
			
			if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
				intensity = aa_intensity(outer_edge);
			} else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
				intensity = aa_intensity(inner_edge);
			} else if (outer_edge < 0.0f && inner_edge < 0.0f) {
				intensity = 1.0f;
			}
			
			int px = x + w - r + cx;
			int py = y + h - r + cy;
			draw_aa_pixel(g, px, py, color, intensity);
		}
	}
}

#else

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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
			}
		}
	}
}

#endif

#ifdef __cplusplus
}
#endif
