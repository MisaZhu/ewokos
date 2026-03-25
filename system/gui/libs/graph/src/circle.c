#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" { 
#endif

// 辅助函数：绘制抗锯齿像素（使用整数alpha）
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

void graph_fill_circle(graph_t* g, int32_t cx, int32_t cy, int32_t radius, uint32_t color) {
    if (radius <= 0 || g == NULL)
        return;
    
    uint8_t fg_alpha = (color >> 24) & 0xFF;
    
    // 整数半径平方
    int32_t r_sq = radius * radius;
    // 抗锯齿边界半径：(radius + 0.5)^2 = radius^2 + radius + 0.25
    // 使用整数：r_aa_sq = radius^2 + radius + 1（向上取整）
    int32_t r_aa_sq = r_sq + radius + 1;
    // 内圈阈值：(radius - 0.5)^2 = radius^2 - radius + 0.25
    int32_t r_inner_sq = r_sq - radius + 1;
    
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
    
    // 按行扫描
    for (int32_t py = min_y; py <= max_y; py++) {
        int32_t dy = py - cy;
        int32_t dy_sq = dy * dy;
        
        // 计算这一行的x范围（考虑抗锯齿区域）
        // x^2 <= r_aa_sq - dy^2
        int32_t x_range_sq = r_aa_sq - dy_sq;
        if (x_range_sq < 0) continue;
        
        int32_t x_range = 0;
        // 整数平方根近似
        while (x_range * x_range <= x_range_sq && x_range <= radius + 1) {
            x_range++;
        }
        x_range--;
        
        int32_t row_min_x = cx - x_range;
        int32_t row_max_x = cx + x_range;
        
        if (row_min_x < min_x) row_min_x = min_x;
        if (row_max_x > max_x) row_max_x = max_x;
        
        // 计算内圈x范围（不需要抗锯齿的部分）
        int32_t inner_x_range_sq = r_inner_sq - dy_sq;
        int32_t inner_min_x = row_min_x;
        int32_t inner_max_x = row_max_x;
        
        if (inner_x_range_sq > 0) {
            int32_t inner_x_range = 0;
            while (inner_x_range * inner_x_range <= inner_x_range_sq && inner_x_range <= radius) {
                inner_x_range++;
            }
            inner_x_range--;
            
            inner_min_x = cx - inner_x_range;
            inner_max_x = cx + inner_x_range;
            
            if (inner_min_x < min_x) inner_min_x = min_x;
            if (inner_max_x > max_x) inner_max_x = max_x;
        }
        
        // 绘制左边界（需要抗锯齿）
        for (int32_t px = row_min_x; px < inner_min_x; px++) {
            int32_t dx = px - cx;
            int32_t dist_sq = dx * dx + dy_sq;
            
            // 计算抗锯齿alpha（整数运算）
            // dist = sqrt(dist_sq), edge_dist = dist - radius
            // intensity = 0.5 - edge_dist = 0.5 - (dist - radius) = radius + 0.5 - dist
            // alpha = fg_alpha * intensity
            
            // 使用整数近似：
            // 当 dist_sq 在 r_inner_sq 和 r_aa_sq 之间时，需要抗锯齿
            if (dist_sq >= r_inner_sq && dist_sq <= r_aa_sq) {
                // 线性插值计算alpha
                // intensity = (r_aa_sq - dist_sq) / (r_aa_sq - r_inner_sq) 近似
                int32_t range = r_aa_sq - r_inner_sq;
                int32_t dist_from_outer = r_aa_sq - dist_sq;
                uint8_t alpha = (uint8_t)((fg_alpha * dist_from_outer + range / 2) / range);
                if (alpha > 0) {
                    draw_aa_pixel_int(g, px, py, color, alpha);
                }
            }
        }
        
        // 判断这一行是否在上下边缘（需要y方向抗锯齿）
        // 当 dy_sq > r_inner_sq 时，说明在上下边缘区域
        int is_top_bottom_edge = (dy_sq > r_inner_sq);
        
        // 绘制内圈
        if (is_top_bottom_edge) {
            // 上下边缘区域：整行都需要抗锯齿
            for (int32_t px = inner_min_x; px <= inner_max_x; px++) {
                int32_t dx = px - cx;
                int32_t dist_sq = dx * dx + dy_sq;
                
                if (dist_sq >= r_inner_sq && dist_sq <= r_aa_sq) {
                    int32_t range = r_aa_sq - r_inner_sq;
                    int32_t dist_from_outer = r_aa_sq - dist_sq;
                    uint8_t alpha = (uint8_t)((fg_alpha * dist_from_outer + range / 2) / range);
                    if (alpha > 0) {
                        draw_aa_pixel_int(g, px, py, color, alpha);
                    }
                }
            }
        } else {
            // 中间区域：不需要抗锯齿
            if (fg_alpha >= 255) {
                for (int32_t px = inner_min_x; px <= inner_max_x; px++) {
                    graph_pixel(g, px, py, color);
                }
            } else if (fg_alpha > 0) {
                uint8_t r = (color >> 16) & 0xFF;
                uint8_t gc = (color >> 8) & 0xFF;
                uint8_t b = color & 0xFF;
                for (int32_t px = inner_min_x; px <= inner_max_x; px++) {
                    graph_pixel_argb(g, px, py, fg_alpha, r, gc, b);
                }
            }
        }
        
        // 绘制右边界（需要抗锯齿）
        for (int32_t px = inner_max_x + 1; px <= row_max_x; px++) {
            int32_t dx = px - cx;
            int32_t dist_sq = dx * dx + dy_sq;
            
            if (dist_sq >= r_inner_sq && dist_sq <= r_aa_sq) {
                int32_t range = r_aa_sq - r_inner_sq;
                int32_t dist_from_outer = r_aa_sq - dist_sq;
                uint8_t alpha = (uint8_t)((fg_alpha * dist_from_outer + range / 2) / range);
                if (alpha > 0) {
                    draw_aa_pixel_int(g, px, py, color, alpha);
                }
            }
        }
    }
}

void graph_circle(graph_t* g, int32_t cx, int32_t cy, int32_t radius, int32_t rw, uint32_t color) {
    if (radius <= 0 || rw <= 0 || g == NULL)
        return;
    
    if (rw > radius / 2) 
        rw = radius / 2;
    
    float outer_r = (float)radius;
    float inner_r = (float)(radius - rw);
    
    // 抗锯齿边界：外圆半径 + 0.5，内圆半径 - 0.5
    float outer_aa_r = outer_r + 0.5f;
    float inner_aa_r = inner_r - 0.5f;
    
    float outer_aa_sq = outer_aa_r * outer_aa_r;
    float outer_r_sq = outer_r * outer_r;
    float inner_r_sq = inner_r * inner_r;
    float inner_aa_sq = inner_aa_r > 0 ? inner_aa_r * inner_aa_r : 0;
    
    // 计算边界框
    int32_t min_y = cy - (int32_t)(outer_aa_r + 0.5f);
    int32_t max_y = cy + (int32_t)(outer_aa_r + 0.5f);
    int32_t min_x = cx - (int32_t)(outer_aa_r + 0.5f);
    int32_t max_x = cx + (int32_t)(outer_aa_r + 0.5f);
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;
    
    // 逐像素扫描
    for (int32_t py = min_y; py <= max_y; py++) {
        float dy = (float)(py - cy);
        float dy_sq = dy * dy;
        
        for (int32_t px = min_x; px <= max_x; px++) {
            float dx = (float)(px - cx);
            float dist_sq = dx * dx + dy_sq;
            
            // 完全在抗锯齿区域外，跳过
            if (dist_sq > outer_aa_sq) continue;
            
            // 完全在内圆内部（圆环空心部分），跳过
            if (dist_sq < inner_aa_sq) continue;
            
            float dist = sqrtf(dist_sq);
            float intensity = 0.0f;
            
            // 外边缘抗锯齿区域
            if (dist >= outer_r - 0.5f && dist <= outer_r + 0.5f) {
                float edge_dist = dist - outer_r;
                intensity = 0.5f - edge_dist; // 线性插值：从外向内 intensity 从 0 到 1
                if (intensity < 0.0f) intensity = 0.0f;
                if (intensity > 1.0f) intensity = 1.0f;
            }
            // 内边缘抗锯齿区域
            else if (dist >= inner_r - 0.5f && dist <= inner_r + 0.5f) {
                float edge_dist = inner_r - dist;
                intensity = 0.5f - edge_dist; // 线性插值：从内向外 intensity 从 0 到 1
                if (intensity < 0.0f) intensity = 0.0f;
                if (intensity > 1.0f) intensity = 1.0f;
            }
            // 圆环内部
            else if (dist > inner_r + 0.5f && dist < outer_r - 0.5f) {
                intensity = 1.0f;
            }
            
            if (intensity > 0.0f) {
                uint8_t fg_alpha = (color >> 24) & 0xFF;
                uint8_t final_alpha = (uint8_t)(fg_alpha * intensity + 0.5f);
                
                if (final_alpha >= 255) {
                    graph_pixel(g, px, py, color);
                } else if (final_alpha > 0) {
                    uint8_t r = (color >> 16) & 0xFF;
                    uint8_t gc = (color >> 8) & 0xFF;
                    uint8_t b = color & 0xFF;
                    graph_pixel_argb(g, px, py, final_alpha, r, gc, b);
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
