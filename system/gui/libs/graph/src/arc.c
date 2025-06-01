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
void graph_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, float start_angle, float end_angle, uint32_t color) {
    if(radius <= 0)
        return;

    if(start_angle > end_angle) {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }

    start_angle = DEG_TO_RAD(start_angle);
    end_angle = DEG_TO_RAD(end_angle);

    if(color_a(color) != 0xff) {
        uint8_t ca = (color >> 24) & 0xff;
        uint8_t cr = (color >> 16) & 0xff;
        uint8_t cg = (color >> 8) & 0xff;
        uint8_t cb = (color) & 0xff;

        for(float angle = start_angle; angle <= end_angle; angle += 0.01) {
            int32_t px = x + (int32_t)(radius * cos(angle));
            int32_t py = y + (int32_t)(radius * sin(angle));
            graph_pixel_argb_safe(g, px, py, ca, cr, cg, cb);
        }
    }
    else {
        for(float angle = start_angle; angle <= end_angle; angle += 0.01) {
            int32_t px = x + (int32_t)(radius * cos(angle));
            int32_t py = y + (int32_t)(radius * sin(angle));
            graph_pixel_safe(g, px, py, color);
        }
    }
}

// 绘制填充圆弧
void graph_fill_arc(graph_t* g, int32_t x, int32_t y, int32_t radius, float start_angle, float end_angle, uint32_t color) {
    if(radius <= 0)
        return;

    if(start_angle > end_angle) {
        float temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }

    start_angle = DEG_TO_RAD(start_angle);
    end_angle = DEG_TO_RAD(end_angle);

    if(color_a(color) != 0xff) {
        uint8_t ca = (color >> 24) & 0xff;
        uint8_t cr = (color >> 16) & 0xff;
        uint8_t cg = (color >> 8) & 0xff;
        uint8_t cb = (color) & 0xff;

        // 遍历每个半径，从 1 到指定半径
        for(int32_t r = 1; r <= radius; r++) {
            for(float angle = start_angle; angle <= end_angle; angle += 0.01) {
                int32_t px = x + (int32_t)(r * cos(angle));
                int32_t py = y + (int32_t)(r * sin(angle));
                graph_pixel_argb_safe(g, px, py, ca, cr, cg, cb);
            }
        }
    }
    else {
        // 遍历每个半径，从 1 到指定半径
        for(int32_t r = 1; r <= radius; r++) {
            for(float angle = start_angle; angle <= end_angle; angle += 0.01) {
                int32_t px = x + (int32_t)(r * cos(angle));
                int32_t py = y + (int32_t)(r * sin(angle));
                graph_pixel_safe(g, px, py, color);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif