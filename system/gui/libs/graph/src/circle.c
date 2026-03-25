#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

void graph_fill_circle(graph_t* g, int32_t cx, int32_t cy, int32_t radius, uint32_t color) {
    if (radius <= 0 || g == NULL)
        return;
    
    // 计算边界框
    int32_t min_y = cy - radius - 1;
    int32_t max_y = cy + radius + 1;
    int32_t min_x = cx - radius - 1;
    int32_t max_x = cx + radius + 1;
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;
    
    float r_f = (float)radius;
    
    for (int32_t py = min_y; py <= max_y; py++) {
        for (int32_t px = min_x; px <= max_x; px++) {
            float dx = (float)(px - cx);
            float dy = (float)(py - cy);
            float dist = sqrtf(dx * dx + dy * dy);
            
            // 计算到圆边界的距离（负值在内部，正值在外部）
            float edge_dist = dist - r_f;
            
            if (edge_dist >= 0.0f) {
                float intensity = aa_intensity(edge_dist);
                draw_aa_pixel(g, px, py, color, intensity);
            }
            else {
                graph_pixel(g, px, py, color);
            }
        }
    }
}

void graph_circle(graph_t* g, int32_t cx, int32_t cy, int32_t radius, int32_t rw, uint32_t color) {
    if (radius <= 0 || rw <= 0 || g == NULL)
        return;
    
    if (rw > radius / 2) 
        rw = radius / 2;
    
    // 计算边界框
    int32_t min_y = cy - radius - 1;
    int32_t max_y = cy + radius + 1;
    int32_t min_x = cx - radius - 1;
    int32_t max_x = cx + radius + 1;
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;
    
    float outer_r = (float)radius;
    float inner_r = (float)(radius - rw);
    
    for (int32_t py = min_y; py <= max_y; py++) {
        for (int32_t px = min_x; px <= max_x; px++) {
            float dx = (float)(px - cx);
            float dy = (float)(py - cy);
            float dist = sqrtf(dx * dx + dy * dy);
            
            // 计算到内外边界的距离
            float outer_edge = dist - outer_r;
            float inner_edge = inner_r - dist;
            
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
            
            draw_aa_pixel(g, px, py, color, intensity);
        }
    }
}

#else

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
				graph_pixel(g, px, py, color);
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
				graph_pixel(g, px, py, color);
			}
		}
	}
}

#endif

#ifdef __cplusplus
}
#endif
