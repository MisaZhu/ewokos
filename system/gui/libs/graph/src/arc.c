#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <openlibm.h>

#ifdef __cplusplus 
extern "C" { 
#endif

// 角度转弧度
#define DEG_TO_RAD(deg) ((deg) * M_PI / 180.0)

// 辅助函数：绘制抗锯齿像素（使用整数alpha）
static inline void draw_aa_pixel_arc_int(graph_t* g, int32_t x, int32_t y, uint32_t color, uint8_t alpha) {
    if (alpha <= 0) return;
    
    if (alpha >= 255) {
        graph_set_pixel(g, x, y, (color & 0x00FFFFFF) | 0xFF000000);
    } else if (alpha > 0) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t gc = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        graph_pixel_argb_raw(g, x, y, alpha, r, gc, b);
    }
}

// 辅助函数：判断角度是否在圆弧范围内（非浮点距离计算版本）
// 返回0表示不在范围内，1表示在范围内
static inline int angle_in_range(float angle, float start_angle, float end_angle, int swap) {
    // 归一化角度到 [0, 2*PI)
    while (angle < 0) angle += 2 * M_PI;
    while (angle >= 2 * M_PI) angle -= 2 * M_PI;
    
    if (swap) {
        // 交换的情况：范围是 start_angle 到 2*PI 或 0 到 end_angle
        if (angle >= start_angle || angle <= end_angle) {
            return 1;
        }
    } else {
        // 正常情况：范围是 start_angle 到 end_angle
        if (angle >= start_angle && angle <= end_angle) {
            return 1;
        }
    }
    
    return 0;
}

// 绘制圆弧（非浮点距离计算版本）
void graph_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, int32_t rw, float start_angle, float end_angle, uint32_t color) {
    if(radius <= 0 || rw <= 0 || g == NULL)
        return;
    if(rw > radius/2) rw = radius/2;

    int swap = 0;
    if(start_angle > end_angle) {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
        swap = 1;
    }

    float start_rad = DEG_TO_RAD(start_angle);
    float end_rad = DEG_TO_RAD(end_angle);

    // 整数半径和抗锯齿边界
    int32_t outer_r = radius;
    int32_t inner_r = radius - rw;
    int32_t outer_r_sq = outer_r * outer_r;
    int32_t inner_r_sq = inner_r * inner_r;
    
    // 抗锯齿边界（扩大到1像素宽度以获得更平滑的效果）
    int32_t outer_aa_sq = outer_r_sq + 2 * outer_r;  // (r + 1)^2
    int32_t inner_aa_sq = (inner_r > 0) ? inner_r_sq - 2 * inner_r + 1 : 0;  // (inner_r - 1)^2
    if (inner_aa_sq < 0) inner_aa_sq = 0;
    
    // 计算边界框（包含抗锯齿区域）
    int32_t min_y = y - radius - 1;
    int32_t max_y = y + radius + 1;
    int32_t min_x = x - radius - 1;
    int32_t max_x = x + radius + 1;
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;

    uint8_t fg_alpha = (color >> 24) & 0xFF;

    // 逐像素扫描
    for(int32_t py = min_y; py <= max_y; py++) {
        int32_t dy = py - y;
        int32_t dy_sq = dy * dy;
        
        for(int32_t px = min_x; px <= max_x; px++) {
            int32_t dx = px - x;
            int32_t dist_sq = dx * dx + dy_sq;
            
            // 完全在抗锯齿区域外，跳过
            if (dist_sq > outer_aa_sq) continue;
            
            // 完全在内圆内部（空心部分），跳过
            if (dist_sq < inner_aa_sq) continue;
            
            // 检查角度范围
            float angle = atan2f(-dy, dx);
            if (!angle_in_range(angle, start_rad, end_rad, swap)) continue;
            
            uint8_t alpha = 0;
            
            // 外边缘抗锯齿区域
            if (dist_sq >= outer_r_sq && dist_sq <= outer_aa_sq) {
                int32_t range = outer_aa_sq - outer_r_sq;
                int32_t dist_from_outer = outer_aa_sq - dist_sq;
                // 使用平方函数获得更平滑的过渡
                int32_t t = (dist_from_outer * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // 内边缘抗锯齿区域
            else if (dist_sq >= inner_aa_sq && dist_sq <= inner_r_sq) {
                int32_t range = inner_r_sq - inner_aa_sq;
                int32_t dist_from_inner = dist_sq - inner_aa_sq;
                // 使用平方函数获得更平滑的过渡
                int32_t t = (dist_from_inner * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // 圆环内部
            else if (dist_sq > inner_r_sq && dist_sq < outer_r_sq) {
                alpha = fg_alpha;
            }
            
            if (alpha > 0) {
                draw_aa_pixel_arc_int(g, px, py, color, alpha);
            }
        }
    }
}

// 绘制填充圆弧（非浮点距离计算版本）
void graph_fill_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, float start_angle, float end_angle, uint32_t color) {
    if(radius <= 0 || g == NULL)
        return;

    int swap = 0;
    if(start_angle > end_angle) {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
        swap = 1;
    }

    float start_rad = DEG_TO_RAD(start_angle);
    float end_rad = DEG_TO_RAD(end_angle);

    // 整数半径和抗锯齿边界（扩大到1像素宽度以获得更平滑的效果）
    int32_t r_sq = radius * radius;
    int32_t r_aa_sq = r_sq + 2 * radius;  // (r + 1)^2
    int32_t r_inner_sq = (radius > 1) ? (radius - 1) * (radius - 1) : 0;  // (r - 1)^2
    if (r_inner_sq < 0) r_inner_sq = 0;
    
    // 计算边界框（包含抗锯齿区域）
    int32_t min_y = y - radius - 1;
    int32_t max_y = y + radius + 1;
    int32_t min_x = x - radius - 1;
    int32_t max_x = x + radius + 1;
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;

    uint8_t fg_alpha = (color >> 24) & 0xFF;

    // 逐像素扫描
    for(int32_t py = min_y; py <= max_y; py++) {
        int32_t dy = py - y;
        int32_t dy_sq = dy * dy;
        
        for(int32_t px = min_x; px <= max_x; px++) {
            int32_t dx = px - x;
            int32_t dist_sq = dx * dx + dy_sq;
            
            // 完全在抗锯齿区域外，跳过
            if (dist_sq > r_aa_sq) continue;
            
            // 检查角度范围
            float angle = atan2f(-dy, dx);
            if (!angle_in_range(angle, start_rad, end_rad, swap)) continue;

            // 完全在内部区域，直接绘制
            if (dist_sq <= r_inner_sq) {
                if (fg_alpha >= 255) {
                    graph_set_pixel(g, px, py, color);
                } else if (fg_alpha > 0) {
                    uint8_t r = (color >> 16) & 0xFF;
                    uint8_t gc = (color >> 8) & 0xFF;
                    uint8_t b = color & 0xFF;
                    graph_pixel_argb_raw(g, px, py, fg_alpha, r, gc, b);
                }
            }
            // 在边缘抗锯齿区域
            else {
                int32_t range = r_aa_sq - r_inner_sq;
                int32_t dist_from_outer = r_aa_sq - dist_sq;
                // 使用平方函数获得更平滑的过渡
                int32_t t = (dist_from_outer * 256) / range;
                int32_t smoothed = (t * t) / 256;
                uint8_t alpha = (uint8_t)((fg_alpha * smoothed) / 256);
                if (alpha > 0) {
                    draw_aa_pixel_arc_int(g, px, py, color, alpha);
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
