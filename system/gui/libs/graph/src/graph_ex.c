#include <graph/graph_ex.h>
#include <graph/bsp_graph.h>
#include <stdlib.h>
#include <math.h>

uint32_t graph_get_dark_color(uint32_t base) {
        uint32_t a, r, g, b;
        a = (base >> 24) & 0xff;
        r = (base >> 16) & 0xff;
        g = (base >> 8) & 0xff;
        b = base & 0xff;
        return argb(a, (r*2)/3, (g*2)/3, (b*2)/3);
}

uint32_t graph_get_bright_color(uint32_t base) {
        uint32_t a, r, g, b;
        a = (base >> 24) & 0xff;
        r = (base >> 16) & 0xff;
        g = (base >> 8) & 0xff;
        b = base & 0xff;

        r = r<0xAA ? r:0xAA;
        g = g<0xAA ? g:0xAA;
        b = b<0xAA ? b:0xAA;

        return argb(a, (r*4)/3, (g*4)/3, (b*4)/3);
}

void graph_get_3d_color(uint32_t base, uint32_t *dark, uint32_t *bright) {
    if(dark != NULL)
        *dark = graph_get_dark_color(base);
    if(bright != NULL)
        *bright = graph_get_bright_color(base);
}

void graph_fill_3d(graph_t* g, int x, int y, int w, int h, uint32_t color, bool rev) {
        uint32_t dark, bright;
        if(rev)
                graph_get_3d_color(color, &bright, &dark);
        else
                graph_get_3d_color(color, &dark, &bright);

        graph_fill(g, x+1, y+1, w-2, h-2, color);
        graph_box_3d(g, x, y, w, h, bright, dark);
}

void graph_box_3d(graph_t* g,
                int x, int y, int w, int h,
                uint32_t bright_color, uint32_t dark_color) {
        graph_line(g, x, y, x+w-1, y, bright_color);
        graph_line(g, x, y+1, x, y+h-1, bright_color);
        graph_line(g, x+w-1, y, x+w-1, y+h-1, dark_color);
        graph_line(g, x, y+h-1, x+w-1, y+h-1, dark_color);
}

void graph_frame(graph_t* g, int x, int y, int w, int h, int width, uint32_t base_color, bool rev) {
        uint32_t dark, bright;
        if(rev)
                graph_get_3d_color(base_color, &bright, &dark);
        else
                graph_get_3d_color(base_color, &dark, &bright);

        graph_box_3d(g, x, y, w, h, bright, dark);
        for(int i=1; i<(width-1); i++) {
                graph_box(g, x+i, y+i, w-i*2, h-i*2, base_color);
        }
        graph_box_3d(g, x+width-1, y+width-1, w-width*2+2, h-width*2+2, dark, bright);
}

void graph_draw_dot_pattern(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2, uint8_t dspace, uint8_t dw) {
        int i = 0;
        int j = 0;
        bool shift = false;
        grect_t ir = {x, y, w, h};
    if(!graph_insect(g, &ir))
        return;

        x = ir.x;
        y = ir.y;
        w = ir.w;
        h = ir.h;

        graph_fill(g, x, y, w, h, c1);
        while(j < h) {
                while(i < w) {
                        if(dw == 1)
                                graph_set_pixel(g, x+i, y+j, c2);
                        else
                                graph_fill(g, x+i, y+j, dw, dw, c2);
                        i += dw + dspace;
                }
                i = shift ? 0:(dw + dspace)/2;
                shift = !shift;
                j += (dw + dspace);
        }
}

void graph_gradation(graph_t* graph, int x, int y, int w, int h, uint32_t c1, uint32_t c2, bool vertical) {
        int32_t a1 = (c1 >> 24) & 0xff;
        int32_t a2 = (c2 >> 24) & 0xff;
        int32_t r1 = (c1 >> 16) & 0xff;
        int32_t r2 = (c2 >> 16) & 0xff;
        int32_t g1 = (c1 >> 8) & 0xff;
        int32_t g2 = (c2 >> 8) & 0xff;
        int32_t b1 = c1 & 0xff;
        int32_t b2 = c2 & 0xff;

        if(!vertical) {
                        for(int32_t i=1; i<=w; i++) {
                                int32_t a = a1 + (((a2 - a1) * i ) / w);
                                int32_t r = r1 + (((r2 - r1) * i ) / w);
                                int32_t g = g1 + (((g2 - g1) * i ) / w);
                                int32_t b = b1 + (((b2 - b1) * i ) / w);
                                uint32_t c = (uint32_t)((a << 24) | (r << 16) | (g << 8) | b);
                                graph_line(graph, x+i-1, y, x+i-1, y+h-1, c);
                        }
        }
        else {
                        for(int32_t i=1; i<=h; i++) {
                                int32_t a = a1 + (((a2 - a1) * i ) / h);
                                int32_t r = r1 + (((r2 - r1) * i ) / h);
                                int32_t g = g1 + (((g2 - g1) * i ) / h);
                                int32_t b = b1 + (((b2 - b1) * i ) / h);
                                uint32_t c = (uint32_t)((a << 24) | (r << 16) | (g << 8) | b);
                                graph_line(graph, x, y+i-1, x+w-1, y+i-1, c);
                        }
        }
}

void graph_gaussian_cpu(graph_t* g, int x, int y, int w, int h, int r) {
    if (g == NULL || r <= 0) {
        return;
    }

    grect_t ir = {x, y, w, h};
    if(!graph_insect(g, &ir))
        return;
    
    x = ir.x;
    y = ir.y;
    w = ir.w;
    h = ir.h;
    
    // 分配临时缓冲区 (只需要一行大小)
    uint32_t* line_buffer = (uint32_t*)malloc(w * sizeof(uint32_t));
    if (line_buffer == NULL) {
        return;
    }
    
    // 预计算左右边界的偏移量，避免重复计算
    int left_bound = r;
    int right_bound = w - r;
    
    // 水平模糊并存储结果到line_buffer
    for (int iy = 0; iy < h; iy++) {
        int src_y = y + iy;
        
        // 处理左边界区域
        for (int ix = 0; ix < left_bound; ix++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
            uint8_t alpha = 0xFF;
            
            for (int dx = -r; dx <= r; dx++) {
                int nx = ix + dx;
                if (nx >= 0 && nx < w) {
                    uint32_t pixel = graph_get_pixel(g, x + nx, src_y);
                    alpha = (pixel >> 24) & 0xff;
                    sumR += (pixel >> 16) & 0xff;
                    sumG += (pixel >> 8) & 0xff;
                    sumB += pixel & 0xff;
                    count++;
                }
            }
            
            uint8_t avgR = sumR / count;
            uint8_t avgG = sumG / count;
            uint8_t avgB = sumB / count;
            line_buffer[ix] = (alpha << 24) | (avgR << 16) | (avgG << 8) | avgB;
        }
        
        // 处理中间区域 (使用滑动窗口优化)
        for (int ix = left_bound; ix < right_bound; ix++) {
            int sumR = 0, sumG = 0, sumB = 0;
            uint8_t alpha = 0xFF;
            
            // 初始化窗口
            for (int dx = -r; dx <= r; dx++) {
                uint32_t pixel = graph_get_pixel(g, x + ix + dx, src_y);
                alpha = (pixel >> 24) & 0xff;
                sumR += (pixel >> 16) & 0xff;
                sumG += (pixel >> 8) & 0xff;
                sumB += pixel & 0xff;
            }
            
            uint8_t avgR = sumR / (2 * r + 1);
            uint8_t avgG = sumG / (2 * r + 1);
            uint8_t avgB = sumB / (2 * r + 1);
            line_buffer[ix] = (alpha << 24) | (avgR << 16) | (avgG << 8) | avgB;
        }
        
        // 处理右边界区域
        for (int ix = right_bound; ix < w; ix++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
            uint8_t alpha = 0xFF;
            
            for (int dx = -r; dx <= r; dx++) {
                int nx = ix + dx;
                if (nx >= 0 && nx < w) {
                    uint32_t pixel = graph_get_pixel(g, x + nx, src_y);
                    alpha = (pixel >> 24) & 0xff;
                    sumR += (pixel >> 16) & 0xff;
                    sumG += (pixel >> 8) & 0xff;
                    sumB += pixel & 0xff;
                    count++;
                }
            }
            
            uint8_t avgR = sumR / count;
            uint8_t avgG = sumG / count;
            uint8_t avgB = sumB / count;
            line_buffer[ix] = (alpha << 24) | (avgR << 16) | (avgG << 8) | avgB;
        }
        
        // 垂直模糊并直接应用到原图
        for (int ix = 0; ix < w; ix++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
            uint8_t alpha = 0xFF;
            
            for (int dy = -r; dy <= r; dy++) {
                int ny = iy + dy;
                if (ny >= 0 && ny < h) {
                    uint32_t pixel;
                    if (dy == 0) {
                        // 当前行已经在line_buffer中
                        pixel = line_buffer[ix];
                    } else {
                        // 其他行需要重新获取
                        pixel = graph_get_pixel(g, x + ix, y + ny);
                    }
                    
                    alpha = (pixel >> 24) & 0xff;
                    sumR += (pixel >> 16) & 0xff;
                    sumG += (pixel >> 8) & 0xff;
                    sumB += pixel & 0xff;
                    count++;
                }
            }
            
            uint8_t avgR = sumR / count;
            uint8_t avgG = sumG / count;
            uint8_t avgB = sumB / count;
            graph_set_pixel(g, x + ix, y + iy, (alpha << 24) | (avgR << 16) | (avgG << 8) | avgB);
        }
    }
    
    // 释放缓冲区
    free(line_buffer);
}

void graph_gaussian(graph_t* g, int x, int y, int w, int h, int r) {
    if(r > w)
        r = w;
    if(r > h)
        r = h;
#ifdef BSP_BOOST 
    graph_gaussian_bsp(g, x, y, w, h, r);
#else
    graph_gaussian_cpu(g, x, y, w, h, r);
#endif
}

void graph_shadow(graph_t* g, int x, int y, int w, int h, uint8_t shadow, uint32_t color) {
    if(shadow == 0)
        return;
    // 右侧阴影，透明度从左到右逐渐减小
    int rightX = x + w - shadow;
    int rightY = y + shadow;
    int rightW = shadow;
    int rightH = h - shadow;
    uint8_t a =  color_a(color);
    for (int i = 0; i < rightW; ++i) {
        uint8_t alpha = (uint8_t)(a * (rightW-i)/rightW);
        graph_line(g, rightX + i, rightY+i, rightX + i, rightY+i + rightH-(rightW), (alpha << 24) | (0x00FFFFFF & color));
    }

    // 底部阴影，透明度从上到下逐渐减小
    int bottomX = x + shadow;
    int bottomY = y + h - shadow;
    int bottomW = w - shadow*2-1;
    int bottomH = shadow;
    for (int i = 0; i < bottomH; ++i) {
        uint8_t alpha = (uint8_t)(a * (bottomH-i)/bottomH);
        graph_line(g, bottomX+i, bottomY + i, bottomX+ i + bottomW, bottomY + i, (alpha << 24) | (0x00FFFFFF & color));
    }
}

/**
 * 使用普通C实现的毛玻璃效果算法
 * @param args 图像数据，uint32_t ARGB格式
 * @param width 图像宽度
 * @param height 图像高度
 * @param x 处理区域左上角x坐标
 * @param y 处理区域左上角y坐标
 * @param w 处理区域宽度
 * @param h 处理区域高度
 * @param r 模糊半径
 */
void graph_glass_cpu(graph_t* g, int x, int y, int w, int h, int r) {
    uint32_t* args = g->buffer;
    if(args == NULL)
        return;

    grect_t ir = {x, y, w, h};
        if(!graph_insect(g, &ir))
                return;
        x = ir.x;
        y = ir.y;
        w = ir.w;
        h = ir.h;

    // 初始化随机数生成器（使用固定种子确保效果一致）
    srand(0x12345678);
    
    // 处理每个像素
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            // 随机选择一个周围的像素
            int rx = i + (rand() % (2 * r + 1)) - r;
            int ry = j + (rand() % (2 * r + 1)) - r;
            
            // 边界检查
            rx = (rx < x) ? x : ((rx >= x + w) ? x + w - 1 : rx);
            ry = (ry < y) ? y : ((ry >= y + h) ? y + h - 1 : ry);
            
            // 从临时缓冲区中获取随机位置的像素值，并写入原图像
            args[j * g->w + i] = args[ry * g->w + rx];
        }
    }
}

void graph_glass(graph_t* g, int x, int y, int w, int h, int r) {
    if(r > w)
        r = w;
    if(r > h)
        r = h;
#ifdef BSP_BOOST 
    graph_glass_bsp(g, x, y, w, h, r);
#else
    graph_glass_cpu(g, x, y, w, h, r);
#endif
}

#ifdef BSP_BOOST

// 辅助函数：计算抗锯齿强度（基于到理想边界的距离）
static inline float aa_intensity_ex(float dist) {
    if (dist <= -0.5f) return 1.0f;
    if (dist >= 0.5f) return 0.0f;
    return 0.5f - dist;
}

// 辅助函数：绘制抗锯齿像素
static inline void draw_aa_pixel_ex(graph_t* g, int32_t x, int32_t y, uint32_t color, float intensity) {
    if (intensity <= 0.0f) return;
    
    uint8_t fg_alpha = (color >> 24) & 0xFF;
    uint8_t final_alpha = (uint8_t)((fg_alpha * intensity) + 0.5f);
    
    if (final_alpha >= 0xFF) {
        graph_pixel(g, x, y, (color & 0x00FFFFFF) | 0xFF000000);
    } else if (final_alpha > 0) {
        uint8_t r = (color >> 16) & 0xFF;
        uint8_t gc = (color >> 8) & 0xFF;
        uint8_t b = color & 0xFF;
        graph_pixel_argb(g, x, y, final_alpha, r, gc, b);
    }
}

// 辅助函数：根据45度对角线判断颜色
// cx, cy: 相对于圆角中心的坐标 (0,0) 到 (r-1, r-1)
// r: 圆角半径
// upper_color: 对角线左上/上方的颜色
// lower_color: 对角线右下/下方的颜色
static inline uint32_t get_45deg_color(int cx, int cy, int r, uint32_t upper_color, uint32_t lower_color) {
    // 45度对角线: cx - cy = 0 (即 cx = cy)
    // cx - cy <= 0 表示在对角线上方/左侧
    // cx - cy > 0 表示在对角线下方/右侧
    return (cx - cy <= 0) ? upper_color : lower_color;
}

// 3D rounded rectangle function with anti-aliasing
void graph_round_3d(graph_t* g, int x, int y, int w, int h, int r, int rw, uint32_t color, bool reverse) {
    if(w <= 0 || h <= 0 || r < 0)
        return;
    // Limit radius to half of width/height
    if(r > w/2) r = w/2;
    if(r > h/2) r = h/2;
    if(rw > r/2) rw = r/2;
    
    // Calculate 3D effect colors
    uint32_t highlight_color;
    uint32_t deep_color;
    if(reverse) {
        deep_color = graph_get_bright_color(color);
        highlight_color = graph_get_dark_color(color);
    }
    else {
        highlight_color = graph_get_bright_color(color);
        deep_color = graph_get_dark_color(color);
    }
    
    float r_f = (float)r;
    float inner_r_f = (float)(r - rw);
    
    // Draw highlight on top edge
    for(int i = 0; i < rw; i++) {
        int yy = y + i;
        if(yy >= 0 && yy < g->h) {
            for(int xx = x + r; xx < x + w - r; xx++) {
                graph_pixel(g, xx, yy, highlight_color);
            }
        }
    }
    
    // Draw highlight on left edge
    for(int i = 0; i < rw; i++) {
        int xx = x + i;
        if(xx >= 0 && xx < g->w) {
            for(int yy = y + r; yy < y + h - r; yy++) {
                graph_pixel(g, xx, yy, highlight_color);
            }
        }
    }
    
    // Draw shadow on bottom edge
    for(int i = 0; i < rw; i++) {
        int yy = y + h - 1 - i;
        if(yy >= 0 && yy < g->h) {
            for(int xx = x + r; xx < x + w - r; xx++) {
                graph_pixel(g, xx, yy, deep_color);
            }
        }
    }
    
    // Draw shadow on right edge
    for(int i = 0; i < rw; i++) {
        int xx = x + w - 1 - i;
        if(xx >= 0 && xx < g->w) {
            for(int yy = y + r; yy < y + h - r; yy++) {
                graph_pixel(g, xx, yy, deep_color);
            }
        }
    }

    // Draw rounded corners with anti-aliasing and 45-degree split
    
    // Top-left corner (all highlight)
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            float dx = (float)cx + 0.5f;
            float dy = (float)cy + 0.5f;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float outer_edge = dist - r_f;
            float inner_edge = inner_r_f - dist;
            
            float intensity = 0.0f;
            if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
                intensity = aa_intensity_ex(outer_edge);
            } else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
                intensity = aa_intensity_ex(inner_edge);
            } else if (outer_edge < 0.0f && inner_edge < 0.0f) {
                intensity = 1.0f;
            }
            
            int px = x + (r - 1 - cx);
            int py = y + (r - 1 - cy);
            draw_aa_pixel_ex(g, px, py, highlight_color, intensity);
        }
    }

    // Top-right corner (45-degree split: upper-left=highlight, lower-right=deep)
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            float dx = (float)cx + 0.5f;
            float dy = (float)cy + 0.5f;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float outer_edge = dist - r_f;
            float inner_edge = inner_r_f - dist;
            
            float intensity = 0.0f;
            if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
                intensity = aa_intensity_ex(outer_edge);
            } else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
                intensity = aa_intensity_ex(inner_edge);
            } else if (outer_edge < 0.0f && inner_edge < 0.0f) {
                intensity = 1.0f;
            }
            
            int px = x + w - r + cx;
            int py = y + (r - 1 - cy);
            // 45度分割: cx - cy <= 0 是 highlight (左上), > 0 是 deep (右下)
            uint32_t c = get_45deg_color(cx, cy, r, highlight_color, deep_color);
            draw_aa_pixel_ex(g, px, py, c, intensity);
        }
    }

    // Bottom-left corner (45-degree split: upper-left=highlight, lower-right=deep)
    // 注意：这里需要镜像坐标来正确判断45度分割
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            float dx = (float)cx + 0.5f;
            float dy = (float)cy + 0.5f;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float outer_edge = dist - r_f;
            float inner_edge = inner_r_f - dist;
            
            float intensity = 0.0f;
            if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
                intensity = aa_intensity_ex(outer_edge);
            } else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
                intensity = aa_intensity_ex(inner_edge);
            } else if (outer_edge < 0.0f && inner_edge < 0.0f) {
                intensity = 1.0f;
            }
            
            int px = x + (r - 1 - cx);
            int py = y + h - r + cy;
            // 45度分割: 在底部左侧，需要镜像判断
            // 原始 cx 从 0(左) 到 r-1(右), cy 从 0(上) 到 r-1(下)
            // 镜像后: (r-1-cx) 从 r-1(左) 到 0(右)
            // 所以判断条件反转: (r-1-cx) - cy <= 0 即 (r-1) <= cx + cy
            // 简化: 当 cx 大且 cy 小时是 deep 色
            uint32_t c = get_45deg_color(cx, cy, r, deep_color, highlight_color);
            draw_aa_pixel_ex(g, px, py, c, intensity);
        }
    }

    // Bottom-right corner (all deep)
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            float dx = (float)cx + 0.5f;
            float dy = (float)cy + 0.5f;
            float dist = sqrtf(dx * dx + dy * dy);
            
            float outer_edge = dist - r_f;
            float inner_edge = inner_r_f - dist;
            
            float intensity = 0.0f;
            if (outer_edge >= -0.5f && outer_edge <= 0.5f) {
                intensity = aa_intensity_ex(outer_edge);
            } else if (inner_edge >= -0.5f && inner_edge <= 0.5f) {
                intensity = aa_intensity_ex(inner_edge);
            } else if (outer_edge < 0.0f && inner_edge < 0.0f) {
                intensity = 1.0f;
            }
            
            int px = x + w - r + cx;
            int py = y + h - r + cy;
            draw_aa_pixel_ex(g, px, py, deep_color, intensity);
        }
    }
}

#else

void graph_round_3d(graph_t* g, int x, int y, int w, int h, int r, int rw, uint32_t color, bool reverse) {
    if(w <= 0 || h <= 0 || r < 0)
        return;
    // Limit radius to half of width/height
    if(r > w/2) r = w/2;
    if(r > h/2) r = h/2;
    if(rw > r/2) rw = r/2;
    
    // Calculate 3D effect colors
    uint32_t highlight_color;
    uint32_t deep_color;
    if(reverse) {
        deep_color = graph_get_bright_color(color);  // Top/left highlight
        highlight_color = graph_get_dark_color(color);      // Bottom/right shadow
    }
    else {
        highlight_color = graph_get_bright_color(color);  // Top/left highlight
        deep_color = graph_get_dark_color(color);      // Bottom/right shadow
    }
    
    // Draw highlight on top edge
    for(int i = 0; i < rw; i++) {
        int yy = y + i;
        if(yy >= 0 && yy < g->h) {
            for(int xx = x + r; xx < x + w - r; xx++) {
                graph_pixel(g, xx, yy, highlight_color);
            }
        }
    }
    
    // Draw highlight on left edge
    for(int i = 0; i < rw; i++) {
        int xx = x + i;
        if(xx >= 0 && xx < g->w) {
            for(int yy = y + r; yy < y + h - r; yy++) {
                graph_pixel(g, xx, yy, highlight_color);
            }
        }
    }
    
    // Draw shadow on bottom edge
    for(int i = 0; i < rw; i++) {
        int yy = y + h - 1 - i;
        if(yy >= 0 && yy < g->h) {
            for(int xx = x + r; xx < x + w - r; xx++) {
                graph_pixel(g, xx, yy, deep_color);
            }
        }
    }
    
    // Draw shadow on right edge
    for(int i = 0; i < rw; i++) {
        int xx = x + w - 1 - i;
        if(xx >= 0 && xx < g->w) {
            for(int yy = y + r; yy < y + h - r; yy++) {
                graph_pixel(g, xx, yy, deep_color);
            }
        }
    }

    // Draw rounded corners with gradient effect
    // For border width rw, we draw arcs with thickness by checking distance range
    // All corners use positive dx/cy, dy/cy for consistent rounding (same as bottom-right)
    
    // Top-left corner (highlight) - quarter circle arc with thickness
    // Use the same dx=cx, dy=cy as bottom-right for consistent rounding
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            int dx = cx;
            int dy = cy;
            int dist_sq = dx*dx + dy*dy;
            // Draw arc with thickness rw
            if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
                // For top-left, mirror the position from bottom-right pattern
                int px = x + (r - 1 - cx);
                int py = y + (r - 1 - cy);
                graph_pixel(g, px, py, highlight_color);
            }
        }
    }

    // Top-right corner - quarter circle arc with thickness
    // Use the same dx=cx, dy=cy as bottom-right for consistent rounding
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            int dx = cx;
            int dy = cy;
            int dist_sq = dx*dx + dy*dy;
            if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
                // For top-right, mirror vertically from bottom-right pattern
                int px = x + w - r + cx;
                int py = y + (r - 1 - cy);
                    // 45 degree diagonal: cx + cy >= r means lower-right area
                uint32_t c = (cx - cy <= 0) ?  highlight_color : deep_color;
                graph_pixel(g, px, py, c);
            }
        }
    }

    // Bottom-left corner - quarter circle arc with thickness
    // Use the same dx=cx, dy=cy as bottom-right for consistent rounding
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            int dx = cx;
            int dy = cy;
            int dist_sq = dx*dx + dy*dy;
            if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
                // For bottom-left, mirror horizontally from bottom-right pattern
                int px = x + (r - 1 - cx);
                int py = y + h - r + cy;
                    // 45 degree diagonal: cx + cy >= r means lower-right area
                uint32_t c = (cx - cy <= 0) ?  deep_color : highlight_color;
                graph_pixel(g, px, py, c);
            }
        }
    }

    // Bottom-right corner (shadow) - quarter circle arc with thickness
    // Original pattern that all others mirror
    for(int cy = 0; cy < r; cy++) {
        for(int cx = 0; cx < r; cx++) {
            int dx = cx;
            int dy = cy;
            int dist_sq = dx*dx + dy*dy;
            if(dist_sq >= (r-rw)*(r-rw) && dist_sq <= r*r) {
                int px = x + w - r + cx;
                int py = y + h - r + cy;
                graph_pixel(g, px, py, deep_color);
            }
        }
    }
}

#endif

// 3D rounded rectangle function
void graph_fill_round_3d(graph_t* g, int x, int y, int w, int h, int r, int rw, uint32_t color, bool reverse) {
    if(w <= 0 || h <= 0 || r < 0)
        return;
    // Limit radius to half of width/height
    if(r > w/2) r = w/2;
    if(r > h/2) r = h/2;
    if(rw > r/2) rw = r/2;
    
    // Draw main rounded rectangle with base color
    graph_fill_round(g, x+rw, y+rw, w-2*rw, h-2*rw, r-rw, color);
    graph_round_3d(g, x, y, w, h, r, rw, color, reverse);
}

void graph_circle_3d(graph_t* g, int x, int y, int r, int rw, uint32_t color, bool reverse) {
    graph_round_3d(g, x-r, y-r, r*2, r*2, r, rw, color, reverse);
}

void graph_fill_circle_3d(graph_t* g, int x, int y, int r, int rw, uint32_t color, bool reverse) {
    graph_fill_round_3d(g, x-r, y-r, r*2, r*2, r, rw, color, reverse);
}
