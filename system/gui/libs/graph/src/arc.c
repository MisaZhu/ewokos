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

#ifdef BSP_BOOST

// 辅助函数：计算抗锯齿强度
static inline float aa_intensity_arc(float dist) {
    if (dist <= -0.5f) return 1.0f;
    if (dist >= 0.5f) return 0.0f;
    return 0.5f - dist;
}

// 辅助函数：绘制抗锯齿像素
static inline void draw_aa_pixel_arc(graph_t* g, int32_t x, int32_t y, uint32_t color, float intensity) {
    if (intensity <= 0.0f) return;
    
    uint8_t fg_alpha = (color >> 24) & 0xFF;
    uint8_t final_alpha = (uint8_t)((fg_alpha * intensity) + 0.5f);
    
    if (final_alpha >= 0xFF) {
        graph_set_pixel(g, x, y, (color & 0x00FFFFFF) | 0xFF000000);
    } else if (final_alpha > 0) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t gc = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        graph_pixel_argb_raw(g, x, y, final_alpha, r, gc, b);
    }
}

// 辅助函数：判断角度是否在圆弧范围内，并返回角度边缘的抗锯齿强度
// 返回0表示不在范围内，>0表示强度
static inline float angle_intensity(float angle, float start_angle, float end_angle, int swap) {
    // 归一化角度到 [0, 2*PI)
    while (angle < 0) angle += 2 * M_PI;
    while (angle >= 2 * M_PI) angle -= 2 * M_PI;
    
    float in_range = 0;
    
    if (swap) {
        // 交换的情况：范围是 start_angle 到 2*PI 或 0 到 end_angle
        if (angle >= start_angle || angle <= end_angle) {
            in_range = 1.0f;
        }
    } else {
        // 正常情况：范围是 start_angle 到 end_angle
        if (angle >= start_angle && angle <= end_angle) {
            in_range = 1.0f;
        }
    }
    
    // 如果在范围内，检查是否在边缘附近需要抗锯齿
    const float edge_width = 0.02f; // 约1度的弧度
    
    if (in_range > 0) {
        float dist_to_start, dist_to_end;
        
        if (swap) {
            // 交换情况下，start_angle 是结束边缘，end_angle 是开始边缘
            if (angle >= start_angle) {
                dist_to_start = angle - start_angle;
            } else {
                dist_to_start = (2 * M_PI - start_angle) + angle;
            }
            if (angle <= end_angle) {
                dist_to_end = end_angle - angle;
            } else {
                dist_to_end = (end_angle) + (2 * M_PI - angle);
            }
        } else {
            dist_to_start = angle - start_angle;
            dist_to_end = end_angle - angle;
        }
        
        // 检查是否在角度边缘
        if (dist_to_start < edge_width) {
            return dist_to_start / edge_width;
        } else if (dist_to_end < edge_width) {
            return dist_to_end / edge_width;
        }
        return 1.0f;
    }
    
    return 0.0f;
}

// 绘制圆弧
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

    float outer_r = (float)radius;
    float inner_r = (float)(radius - rw);
    // 抗锯齿边界：半径 + 0.5，确保边缘像素都被处理
    float aa_outer_r = outer_r + 0.5f;
    float aa_outer_r_sq = aa_outer_r * aa_outer_r;
    
    // 计算边界框（包含抗锯齿区域）
    int32_t min_y = y - (int32_t)(aa_outer_r + 0.5f);
    int32_t max_y = y + (int32_t)(aa_outer_r + 0.5f);
    int32_t min_x = x - (int32_t)(aa_outer_r + 0.5f);
    int32_t max_x = x + (int32_t)(aa_outer_r + 0.5f);
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;

    // 按行扫描优化
    for(int32_t py = min_y; py <= max_y; py++) {
        float cy = (float)(py - y);
        float cy_sq = cy * cy;
        
        // 计算这一行在外圆内的x范围
        float outer_x_sq = aa_outer_r_sq - cy_sq;
        if (outer_x_sq < 0) continue; // 这一行在圆外
        
        float outer_x = sqrtf(outer_x_sq);
        int32_t row_min_x = x - (int32_t)(outer_x + 0.5f);
        int32_t row_max_x = x + (int32_t)(outer_x + 0.5f);
        
        if (row_min_x < min_x) row_min_x = min_x;
        if (row_max_x > max_x) row_max_x = max_x;
        
        // 计算这一行在内圆内的x范围
        float inner_x_sq = inner_r * inner_r - cy_sq;
        int32_t inner_row_min_x = row_min_x;
        int32_t inner_row_max_x = row_max_x;
        
        if (inner_x_sq > 0) {
            float inner_x = sqrtf(inner_x_sq);
            inner_row_min_x = x - (int32_t)(inner_x + 0.5f);
            inner_row_max_x = x + (int32_t)(inner_x + 0.5f);
            
            if (inner_row_min_x < min_x) inner_row_min_x = min_x;
            if (inner_row_max_x > max_x) inner_row_max_x = max_x;
        }
        
        // 绘制左外边界（抗锯齿）
        for (int32_t px = row_min_x; px < inner_row_min_x; px++) {
            float cx = (float)(px - x);
            float dist = sqrtf(cx*cx + cy_sq);
            float outer_edge = dist - outer_r;
            float radial_intensity = aa_intensity_arc(outer_edge);
            
            if (radial_intensity > 0.0f) {
                float angle = atan2f(cy, cx);
                float angle_intens = angle_intensity(angle, start_rad, end_rad, swap);
                if (angle_intens > 0.0f) {
                    float final_intensity = radial_intensity * angle_intens;
                    draw_aa_pixel_arc(g, px, py, color, final_intensity);
                }
            }
        }
        
        // 绘制圆环内部（需要检查角度）
        for (int32_t px = inner_row_min_x; px <= inner_row_max_x; px++) {
            float cx = (float)(px - x);
            float dist = sqrtf(cx*cx + cy_sq);
            float outer_edge = dist - outer_r;
            float inner_edge = inner_r - dist;
            
            float radial_intensity = 0.0f;
            if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
                radial_intensity = aa_intensity_arc(outer_edge);
            } else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
                radial_intensity = aa_intensity_arc(inner_edge);
            } else if (outer_edge < 0.0f && inner_edge < 0.0f) {
                radial_intensity = 1.0f;
            }
            
            if (radial_intensity > 0.0f) {
                float angle = atan2f(cy, cx);
                float angle_intens = angle_intensity(angle, start_rad, end_rad, swap);
                if (angle_intens > 0.0f) {
                    float final_intensity = radial_intensity * angle_intens;
                    draw_aa_pixel_arc(g, px, py, color, final_intensity);
                }
            }
        }
        
        // 绘制右外边界（抗锯齿）
        for (int32_t px = inner_row_max_x + 1; px <= row_max_x; px++) {
            float cx = (float)(px - x);
            float dist = sqrtf(cx*cx + cy_sq);
            float outer_edge = dist - outer_r;
            float radial_intensity = aa_intensity_arc(outer_edge);
            
            if (radial_intensity > 0.0f) {
                float angle = atan2f(cy, cx);
                float angle_intens = angle_intensity(angle, start_rad, end_rad, swap);
                if (angle_intens > 0.0f) {
                    float final_intensity = radial_intensity * angle_intens;
                    draw_aa_pixel_arc(g, px, py, color, final_intensity);
                }
            }
        }
    }
}

// 绘制填充圆弧
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

    float r_f = (float)radius;
    // 抗锯齿边界：半径 + 0.5，确保边缘像素都被处理
    float aa_radius = r_f + 0.5f;
    float aa_radius_sq = aa_radius * aa_radius;
    
    // 计算边界框（包含抗锯齿区域）
    int32_t min_y = y - (int32_t)(aa_radius + 0.5f);
    int32_t max_y = y + (int32_t)(aa_radius + 0.5f);
    int32_t min_x = x - (int32_t)(aa_radius + 0.5f);
    int32_t max_x = x + (int32_t)(aa_radius + 0.5f);
    
    // 裁剪到画布边界
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;

    // 按行扫描优化
    for(int32_t py = min_y; py <= max_y; py++) {
        float cy = (float)(py - y);
        float cy_sq = cy * cy;
        
        // 计算这一行的x范围（考虑抗锯齿区域）
        float x_range_sq = aa_radius_sq - cy_sq;
        if (x_range_sq < 0) continue; // 这一行在圆外
        
        float x_range = sqrtf(x_range_sq);
        int32_t row_min_x = x - (int32_t)(x_range + 0.5f);
        int32_t row_max_x = x + (int32_t)(x_range + 0.5f);
        
        if (row_min_x < min_x) row_min_x = min_x;
        if (row_max_x > max_x) row_max_x = max_x;
        
        // 绘制这一行的每个像素
        for (int32_t px = row_min_x; px <= row_max_x; px++) {
            float cx = (float)(px - x);
            float dist = sqrtf(cx*cx + cy_sq);
            float edge_dist = dist - r_f;
            float radial_intensity = aa_intensity_arc(edge_dist);
            
            if (radial_intensity > 0.0f) {
                float angle = atan2f(cy, cx);
                float angle_intens = angle_intensity(angle, start_rad, end_rad, swap);
                
                if (angle_intens > 0.0f) {
                    float final_intensity = radial_intensity * angle_intens;
                    draw_aa_pixel_arc(g, px, py, color, final_intensity);
                }
            }
        }
    }
}

#else

// 绘制圆弧
void graph_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, int32_t rw, float start_angle, float end_angle, uint32_t color) {
    if(radius <= 0 || rw <= 0)
        return;
    if(rw > radius/2) rw = radius/2;

    int swap = 0;
    if(start_angle > end_angle) {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
        swap = 1;
    }

    start_angle = DEG_TO_RAD(start_angle);
    end_angle = DEG_TO_RAD(end_angle);

    int32_t outer_r = radius + 1;
    int32_t outer_r_sq = outer_r * outer_r - 1;
    
    int32_t inner_r = radius - rw;
    int32_t inner_r_adj = inner_r;
    int32_t inner_r_sq = inner_r_adj * inner_r_adj;

    for(int cy = -radius - 1; cy <= radius + 1; cy++) {
        for(int cx = -radius - 1; cx <= radius + 1; cx++) {
            int dist_sq = cx*cx + cy*cy;
            
            if(dist_sq >= inner_r_sq && dist_sq <= outer_r_sq) {
                float angle = atan2f(cy, cx);
                if(angle < 0) angle += 2 * M_PI;
                
                int in_range = 0;
                if(swap) {
                    if(angle >= start_angle || angle <= end_angle) {
                        in_range = 1;
                    }
                }
                else {
                    if(angle >= start_angle && angle <= end_angle) {
                        in_range = 1;
                    }
                }
                
                if(in_range) {
                    int px = x + cx;
                    int py = y + cy;
                    graph_pixel(g, px, py, color);
                }
            }
        }
    }
}

// 绘制填充圆弧
void graph_fill_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, float start_angle, float end_angle, uint32_t color) {
    if(radius <= 0)
        return;

    int swap = 0;
    if(start_angle > end_angle) {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
        swap = 1;
    }

    start_angle = DEG_TO_RAD(start_angle);
    end_angle = DEG_TO_RAD(end_angle);

    int32_t r_plus_half = radius + 1;
    int32_t r_sq_plus = (r_plus_half * r_plus_half) - 1;

    for(int cy = -radius - 1; cy <= radius + 1; cy++) {
        for(int cx = -radius - 1; cx <= radius + 1; cx++) {
            int dist_sq = cx*cx + cy*cy;
            
            if(dist_sq <= r_sq_plus) {
                float angle = atan2f(cy, cx);
                if(angle < 0) angle += 2 * M_PI;
                
                int in_range = 0;
                if(swap) {
                    if(angle >= start_angle || angle <= end_angle) {
                        in_range = 1;
                    }
                }
                else {
                    if(angle >= start_angle && angle <= end_angle) {
                        in_range = 1;
                    }
                }
                
                if(in_range) {
                    int px = x + cx;
                    int py = y + cy;
                    graph_pixel(g, px, py, color);
                }
            }
        }
    }
}

#endif

#ifdef __cplusplus
}
#endif
