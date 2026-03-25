#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" { 
#endif

#ifdef BSP_BOOST

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

// 辅助函数：绘制圆角（按行扫描优化）
// corner_x, corner_y: 圆角矩形区域的起始坐标
// cx, cy: 圆角中心相对于corner的偏移
// r: 圆角半径
static inline void draw_round_corner_fill(graph_t* g, int32_t corner_x, int32_t corner_y, 
                                          int32_t cx, int32_t cy, int32_t r, 
                                          uint32_t color, int mirror_x, int mirror_y) {
    float r_f = (float)r;
    float r_sq = r_f * r_f;
    // 抗锯齿边界：半径 + 0.5，确保边缘像素都被处理
    float aa_radius = r_f + 0.5f;
    float aa_radius_sq = aa_radius * aa_radius;
    
    // 计算边界框（相对于圆角中心）
    int32_t min_dy = 0;
    int32_t max_dy = (int32_t)(aa_radius + 0.5f);
    
    // 按行扫描
    for (int32_t dy = min_dy; dy <= max_dy; dy++) {
        float dy_f = (float)dy + 0.5f;
        float dy_sq = dy_f * dy_f;
        
        // 计算这一行的x范围
        float x_range_sq = aa_radius_sq - dy_sq;
        if (x_range_sq < 0) continue;
        
        float x_range = sqrtf(x_range_sq);
        int32_t max_dx = (int32_t)(x_range + 0.5f);
        
        // 绘制这一行
        for (int32_t dx = 0; dx <= max_dx; dx++) {
            float dx_f = (float)dx + 0.5f;
            float dist = sqrtf(dx_f * dx_f + dy_sq);
            float edge_dist = dist - r_f;
            float intensity = aa_intensity(edge_dist);
            
            if (intensity > 0.0f) {
                int px = corner_x + cx + (mirror_x ? -dx : dx);
                int py = corner_y + cy + (mirror_y ? -dy : dy);
                draw_aa_pixel(g, px, py, color, intensity);
            }
        }
    }
}

// 辅助函数：绘制圆角边框（按行扫描优化）
static inline void draw_round_corner_round(graph_t* g, int32_t corner_x, int32_t corner_y, 
                                           int32_t cx, int32_t cy, int32_t r, int32_t rw,
                                           uint32_t color, int mirror_x, int mirror_y) {
    float r_f = (float)r;
    float inner_r_f = (float)(r - rw);
    float aa_radius = r_f + 0.5f;
    float aa_radius_sq = aa_radius * aa_radius;
    
    // 计算边界框
    int32_t min_dy = 0;
    int32_t max_dy = (int32_t)(aa_radius + 0.5f);
    
    // 按行扫描
    for (int32_t dy = min_dy; dy <= max_dy; dy++) {
        float dy_f = (float)dy + 0.5f;
        float dy_sq = dy_f * dy_f;
        
        // 计算这一行在外圆内的x范围
        float outer_x_sq = aa_radius_sq - dy_sq;
        if (outer_x_sq < 0) continue;
        
        float outer_x = sqrtf(outer_x_sq);
        int32_t max_dx = (int32_t)(outer_x + 0.5f);
        
        // 计算这一行在内圆内的x范围
        float inner_x_sq = inner_r_f * inner_r_f - dy_sq;
        int32_t inner_max_dx = 0;
        if (inner_x_sq > 0) {
            inner_max_dx = (int32_t)(sqrtf(inner_x_sq) + 0.5f);
        }
        
        // 绘制左外边界（抗锯齿）
        for (int32_t dx = inner_max_dx + 1; dx <= max_dx; dx++) {
            float dx_f = (float)dx + 0.5f;
            float dist = sqrtf(dx_f * dx_f + dy_sq);
            float outer_edge = dist - r_f;
            float intensity = aa_intensity(outer_edge);
            
            if (intensity > 0.0f) {
                int px = corner_x + cx + (mirror_x ? -dx : dx);
                int py = corner_y + cy + (mirror_y ? -dy : dy);
                draw_aa_pixel(g, px, py, color, intensity);
            }
        }
        
        // 绘制圆环内部
        for (int32_t dx = 0; dx <= inner_max_dx; dx++) {
            float dx_f = (float)dx + 0.5f;
            float dist = sqrtf(dx_f * dx_f + dy_sq);
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
            
            if (intensity > 0.0f) {
                int px = corner_x + cx + (mirror_x ? -dx : dx);
                int py = corner_y + cy + (mirror_y ? -dy : dy);
                draw_aa_pixel(g, px, py, color, intensity);
            }
        }
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

	// Draw four corners with anti-aliasing (按行扫描优化)
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
	
	// Draw four corners with anti-aliasing (按行扫描优化)
	// Top-left corner
	draw_round_corner_round(g, x, y, r-1, r-1, r, rw, color, 1, 1);
	
	// Top-right corner
	draw_round_corner_round(g, x+w-r, y, 0, r-1, r, rw, color, 0, 1);
	
	// Bottom-left corner
	draw_round_corner_round(g, x, y+h-r, r-1, 0, r, rw, color, 1, 0);
	
	// Bottom-right corner
	draw_round_corner_round(g, x+w-r, y+h-r, 0, 0, r, rw, color, 0, 0);
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
