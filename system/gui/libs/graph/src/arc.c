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
                    if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
                        graph_pixel_alpha(g, px, py, color);
                    }
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
                    if(px >= 0 && px < g->w && py >= 0 && py < g->h) {
                        graph_pixel_alpha(g, px, py, color);
                    }
                }
            }
        }
    }
}

#ifdef __cplusplus
}
#endif