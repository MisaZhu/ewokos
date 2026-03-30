#include <graph/graph_ex.h>
#include <graph/bsp_graph.h>
#include <stdlib.h>
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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
    
    // Allocate temporary buffer (only need one line size)
    uint32_t* line_buffer = (uint32_t*)malloc(w * sizeof(uint32_t));
    if (line_buffer == NULL) {
        return;
    }
    
    // Pre-calculate left and right boundary offsets to avoid repeated calculation
    int left_bound = r;
    int right_bound = w - r;
    
    // Horizontal blur and store result to line_buffer
    for (int iy = 0; iy < h; iy++) {
        int src_y = y + iy;
        
        // Process left boundary area
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
        
        // Process middle area (using sliding window optimization)
        for (int ix = left_bound; ix < right_bound; ix++) {
            int sumR = 0, sumG = 0, sumB = 0;
            uint8_t alpha = 0xFF;
            
            // Initialize window
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
        
        // Process right boundary area
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
        
        // Vertical blur and apply directly to original image
        for (int ix = 0; ix < w; ix++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;
            uint8_t alpha = 0xFF;
            
            for (int dy = -r; dy <= r; dy++) {
                int ny = iy + dy;
                if (ny >= 0 && ny < h) {
                    uint32_t pixel;
                    if (dy == 0) {
                        // Current line is already in line_buffer
                        pixel = line_buffer[ix];
                    } else {
                        // Other lines need to be re-fetched
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
    
    // Free buffer
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
    // Right shadow, transparency decreases from left to right
    int rightX = x + w - shadow;
    int rightY = y + shadow;
    int rightW = shadow;
    int rightH = h - shadow;
    uint8_t a =  color_a(color);
    for (int i = 0; i < rightW; ++i) {
        uint8_t alpha = (uint8_t)(a * (rightW-i)/rightW);
        graph_line(g, rightX + i, rightY+i, rightX + i, rightY+i + rightH-(rightW), (alpha << 24) | (0x00FFFFFF & color));
    }

    // Bottom shadow, transparency decreases from top to bottom
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

    // Initialize random number generator (use fixed seed for consistent effect)
    srand(0x12345678);
    
    // Process each pixel
    for (int j = y; j < y + h; j++) {
        for (int i = x; i < x + w; i++) {
            // Randomly select a surrounding pixel
            int rx = i + (rand() % (2 * r + 1)) - r;
            int ry = j + (rand() % (2 * r + 1)) - r;
            
            // Boundary check
            rx = (rx < x) ? x : ((rx >= x + w) ? x + w - 1 : rx);
            ry = (ry < y) ? y : ((ry >= y + h) ? y + h - 1 : ry);
            
            // Get pixel value from random position in temporary buffer and write to original image
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

// Helper function: draw anti-aliased pixel (using integer alpha)
static inline void draw_aa_pixel_int_ex(graph_t* g, int32_t x, int32_t y, uint32_t color, uint8_t alpha) {
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

// Helper function: determine color based on 45-degree diagonal
// cx, cy: coordinates relative to corner center (0,0) to (r-1, r-1)
// r: corner radius
// upper_color: color for upper-left/above diagonal
// lower_color: color for lower-right/below diagonal
static inline uint32_t get_45deg_color(int cx, int cy, int r, uint32_t upper_color, uint32_t lower_color) {
    // 45-degree diagonal: cx - cy = 0 (i.e., cx = cy)
    // cx - cy <= 0 means above/left of diagonal
    // cx - cy > 0 means below/right of diagonal
    return (cx - cy <= 0) ? upper_color : lower_color;
}

// Helper function: draw 3D rounded corner (non-floating point implementation with 45-degree split)
// corner_x, corner_y: starting coordinates of corner rectangle area
// cx, cy: offset of corner center relative to corner
// r: corner radius
// rw: border width
// upper_color: color for upper-left/above diagonal
// lower_color: color for lower-right/below diagonal
// swap_45deg: whether to swap 45-degree split judgment (for bottom-left corner)
static inline void draw_round_corner_3d(graph_t* g, int32_t corner_x, int32_t corner_y,
                                        int32_t cx, int32_t cy, int32_t r, int32_t rw,
                                        uint32_t upper_color, uint32_t lower_color,
                                        int mirror_x, int mirror_y, bool swap_45deg) {
    uint8_t upper_alpha = (upper_color >> 24) & 0xFF;
    uint8_t lower_alpha = (lower_color >> 24) & 0xFF;

    // Outer and inner circle radius squared
    int32_t outer_r_sq = r * r;
    int32_t inner_r = r - rw;
    int32_t inner_r_sq = inner_r * inner_r;

    // Anti-aliasing boundary (expanded to 1 pixel width for smoother effect)
    int32_t outer_aa_sq = outer_r_sq + 2 * r;  // (r + 1)^2
    int32_t inner_aa_sq = (inner_r > 0) ? inner_r_sq - 2 * inner_r + 1 : 0;  // (inner_r - 1)^2
    if (inner_r <= 0) inner_aa_sq = 0;

    // Calculate bounding box, limited to r-1 range to align with edges
    int32_t max_dy = r - 1;

    // Scan line by line
    for (int32_t dy = 0; dy <= max_dy; dy++) {
        int32_t dy_sq = dy * dy;

        // Calculate x range for this line within outer circle
        int32_t outer_x_sq = outer_aa_sq - dy_sq;
        if (outer_x_sq < 0) continue;

        int32_t max_dx = 0;
        while (max_dx * max_dx <= outer_x_sq && max_dx <= r - 1) {
            max_dx++;
        }
        max_dx--;
        if (max_dx < 0) max_dx = 0;

        // Calculate x range for this line within inner circle
        int32_t inner_x_sq = inner_r_sq - dy_sq;
        int32_t inner_max_dx = 0;
        if (inner_x_sq > 0) {
            while (inner_max_dx * inner_max_dx <= inner_x_sq && inner_max_dx <= inner_r) {
                inner_max_dx++;
            }
            inner_max_dx--;
        }

        // Draw this line
        for (int32_t dx = 0; dx <= max_dx; dx++) {
            int32_t dist_sq = dx * dx + dy_sq;

            // Completely outside anti-aliasing area, skip
            if (dist_sq > outer_aa_sq) continue;

            // Completely inside inner circle (hollow part), skip
            if (dist_sq < inner_aa_sq) continue;

            // Calculate pixel position
            int px = corner_x + cx + (mirror_x ? -dx : dx);
            int py = corner_y + cy + (mirror_y ? -dy : dy);

            // Check if within canvas bounds
            if (px < 0 || px >= g->w || py < 0 || py >= g->h) continue;

            // Determine color (45-degree split)
            uint32_t color;
            uint8_t fg_alpha;
            if (swap_45deg) {
                color = get_45deg_color(dx, dy, r, lower_color, upper_color);
                fg_alpha = (dx - dy <= 0) ? lower_alpha : upper_alpha;
            } else {
                color = get_45deg_color(dx, dy, r, upper_color, lower_color);
                fg_alpha = (dx - dy <= 0) ? upper_alpha : lower_alpha;
            }

            uint8_t alpha = 0;

            // Outer edge anti-aliasing area
            if (dist_sq >= outer_r_sq && dist_sq <= outer_aa_sq) {
                int32_t range = outer_aa_sq - outer_r_sq;
                int32_t dist_from_outer = outer_aa_sq - dist_sq;
                // Use square function for smoother transition
                int32_t t = (dist_from_outer * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // Inner edge anti-aliasing area
            else if (dist_sq >= inner_aa_sq && dist_sq <= inner_r_sq) {
                int32_t range = inner_r_sq - inner_aa_sq;
                int32_t dist_from_inner = dist_sq - inner_aa_sq;
                // Use square function for smoother transition
                int32_t t = (dist_from_inner * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // Inside ring (completely between inner and outer circles)
            else if (dist_sq > inner_r_sq && dist_sq < outer_r_sq) {
                alpha = fg_alpha;
            }

            if (alpha > 0) {
                draw_aa_pixel_int_ex(g, px, py, color, alpha);
            }
        }
    }
}

// 3D rounded rectangle function with anti-aliasing (non-floating point implementation)
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

    // Draw rounded corners with anti-aliasing and 45-degree split (non-floating point implementation)

    // Top-left corner (all highlight)
    draw_round_corner_3d(g, x, y, r-1, r-1, r, rw, highlight_color, highlight_color, 1, 1, false);

    // Top-right corner (45-degree split: upper-left=highlight, lower-right=deep)
    draw_round_corner_3d(g, x+w-r, y, 0, r-1, r, rw, highlight_color, deep_color, 0, 1, false);

    // Bottom-left corner (45-degree split: upper-left=highlight, lower-right=deep)
    // Need swap_45deg because coordinates are mirrored
    draw_round_corner_3d(g, x, y+h-r, r-1, 0, r, rw, highlight_color, deep_color, 1, 0, true);

    // Bottom-right corner (all deep)
    draw_round_corner_3d(g, x+w-r, y+h-r, 0, 0, r, rw, deep_color, deep_color, 0, 0, false);
}

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
    if(r <= 0 || rw <= 0 || g == NULL)
        return;
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

    uint8_t fg_alpha = (color >> 24) & 0xFF;

    // Outer and inner circle radius squared
    int32_t outer_r_sq = r * r;
    int32_t inner_r = r - rw;
    int32_t inner_r_sq = inner_r * inner_r;

    // Anti-aliasing boundary (expanded to 1 pixel width for smoother effect)
    int32_t outer_aa_sq = outer_r_sq + 2 * r;  // (r + 1)^2
    int32_t inner_aa_sq = (inner_r > 0) ? inner_r_sq - 2 * inner_r + 1 : 0;  // (inner_r - 1)^2
    if (inner_r <= 0) inner_aa_sq = 0;

    // Calculate bounding box
    int32_t min_y = y - r - 1;
    int32_t max_y = y + r + 1;
    int32_t min_x = x - r - 1;
    int32_t max_x = x + r + 1;

    // Clip to canvas bounds
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;

    // Scan pixel by pixel
    for (int32_t py = min_y; py <= max_y; py++) {
        int32_t dy = py - y;
        int32_t dy_sq = dy * dy;

        for (int32_t px = min_x; px <= max_x; px++) {
            int32_t dx = px - x;
            int32_t dist_sq = dx * dx + dy_sq;

            // Completely outside anti-aliasing area, skip
            if (dist_sq > outer_aa_sq) continue;

            // Completely inside inner circle (hollow part), skip
            if (dist_sq < inner_aa_sq) continue;

            uint8_t alpha = 0;

            // Outer edge anti-aliasing area
            if (dist_sq >= outer_r_sq && dist_sq <= outer_aa_sq) {
                int32_t range = outer_aa_sq - outer_r_sq;
                int32_t dist_from_outer = outer_aa_sq - dist_sq;
                int32_t t = (dist_from_outer * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // Inner edge anti-aliasing area
            else if (dist_sq >= inner_aa_sq && dist_sq <= inner_r_sq) {
                int32_t range = inner_r_sq - inner_aa_sq;
                int32_t dist_from_inner = dist_sq - inner_aa_sq;
                int32_t t = (dist_from_inner * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // Inside ring (completely between inner and outer circles)
            else if (dist_sq > inner_r_sq && dist_sq < outer_r_sq) {
                alpha = fg_alpha;
            }

            if (alpha > 0) {
                // Determine color based on 45-degree diagonal
                // For 3D effect with light source from top-left:
                // - Top-left quadrant (dx < 0, dy < 0): highlight
                // - Bottom-right quadrant (dx > 0, dy > 0): deep
                // - Top-right and bottom-left quadrants: split by diagonal
                uint32_t pixel_color;
                if (dx <= 0 && dy <= 0) {
                    // Top-left quadrant: highlight
                    pixel_color = highlight_color;
                } else if (dx >= 0 && dy >= 0) {
                    // Bottom-right quadrant: deep
                    pixel_color = deep_color;
                } else if (dx > 0 && dy < 0) {
                    // Top-right quadrant: split by dx + dy = 0
                    // dx + dy <= 0 means closer to top-left (highlight)
                    pixel_color = (dx + dy <= 0) ? highlight_color : deep_color;
                } else {
                    // Bottom-left quadrant (dx < 0, dy > 0): split by dx + dy = 0
                    // dx + dy <= 0 means closer to top-left (highlight)
                    pixel_color = (dx + dy <= 0) ? highlight_color : deep_color;
                }
                draw_aa_pixel_int_ex(g, px, py, pixel_color, alpha);
            }
        }
    }
}

void graph_fill_circle_3d(graph_t* g, int x, int y, int r, int rw, uint32_t color, bool reverse) {
    if(r <= 0 || g == NULL)
        return;
    if(rw > r/2) rw = r/2;

    // Draw main filled circle with base color
    graph_fill_circle(g, x, y, r - rw, color);
    // Draw 3D border
    graph_circle_3d(g, x, y, r, rw, color, reverse);
}

void graph_semi_round_3d(graph_t* g, int x, int y, int w, int r, int rw, uint32_t color, bool reverse, bool top_half) {
    if(w <= 0 || r < 0)
        return;
    // Limit radius to half of width
    if(r > w/2) r = w/2;
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

    if(top_half) {
        // Top half: draw top edge
        // Draw highlight on top edge
        for(int i = 0; i < rw; i++) {
            int yy = y + i;
            if(yy >= 0 && yy < g->h) {
                for(int xx = x + r; xx < x + w - r; xx++) {
                    graph_pixel(g, xx, yy, highlight_color);
                }
            }
        }

        // Draw two top corners with anti-aliasing and 45-degree split
        // Top-left corner (all highlight)
        draw_round_corner_3d(g, x, y, r-1, r-1, r, rw, highlight_color, highlight_color, 1, 1, false);

        // Top-right corner (45-degree split: upper-left=highlight, lower-right=deep)
        draw_round_corner_3d(g, x+w-r, y, 0, r-1, r, rw, highlight_color, deep_color, 0, 1, false);
    } else {
        // Bottom half: draw bottom edge
        // Draw shadow on bottom edge
        for(int i = 0; i < rw; i++) {
            int yy = y + r - 1 - i;
            if(yy >= 0 && yy < g->h) {
                for(int xx = x + r; xx < x + w - r; xx++) {
                    graph_pixel(g, xx, yy, deep_color);
                }
            }
        }

        // Draw two bottom corners with anti-aliasing and 45-degree split
        // Bottom-left corner (45-degree split: upper-left=highlight, lower-right=deep)
        draw_round_corner_3d(g, x, y, r-1, 0, r, rw, highlight_color, deep_color, 1, 0, true);

        // Bottom-right corner (all deep)
        draw_round_corner_3d(g, x+w-r, y, 0, 0, r, rw, deep_color, deep_color, 0, 0, false);
    }
}

void graph_semi_fill_round_3d(graph_t* g, int x, int y, int w, int r, int rw, uint32_t color, bool reverse, bool top_half) {
    if(w <= 0 || r < 0)
        return;
    // Limit radius to half of width
    if(r > w/2) r = w/2;
    if(rw > r/2) rw = r/2;

    // Draw main semi rounded rectangle with base color
    // Fill area needs to align with border, border width is rw
    // Fill corner radius is r - rw
    if(top_half) {
        // Top half: fill starts from y+rw
        graph_semi_fill_round(g, x+rw, y+rw, w-2*rw, r-rw, color, top_half);
    } else {
        // Bottom half: fill starts from y, but corner radius is r-rw
        graph_semi_fill_round(g, x+rw, y, w-2*rw, r-rw, color, top_half);
    }
    graph_semi_round_3d(g, x, y, w, r, rw, color, reverse, top_half);
}

/**
 * Draw a ring (annulus) arc outline with specified start angle, end angle, radius and thickness
 * Only draws the outline (inner and outer arc lines), does not fill the ring area
 * @param g Graph context
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Outer radius of the ring
 * @param thickness Thickness of the ring (inner radius = radius - thickness)
 * @param start_angle Start angle in degrees (0-360, 0 is at 3 o'clock, counter-clockwise)
 * @param end_angle End angle in degrees (0-360)
 * @param color Ring color
 */
void graph_ring_arc(graph_t* g, int cx, int cy, int radius, int thickness, int rw,
                    int start_angle, int end_angle, uint32_t color) {
    if (g == NULL || radius <= 0 || thickness <= 0 || thickness > radius)
        return;

    int inner_radius = radius - thickness;
    if (inner_radius < 0) inner_radius = 0;

    // Draw outer arc (line width = 1)
    graph_arc(g, cx, cy, radius, rw, (float)start_angle, (float)end_angle, color);
    // Draw inner arc
    if (inner_radius > 0) {
        graph_arc(g, cx, cy, inner_radius, rw, (float)start_angle, (float)end_angle, color);
    }

    
    // Draw radial lines at start and end angles
    // graph_arc uses standard math angles (counter-clockwise from positive X)
    // Screen Y increases downward, so we negate sin: y = cy - r*sin(a)
    if((end_angle - start_angle) == 360)
        return;

    double start_rad = start_angle * M_PI / 180.0;
    double end_rad = end_angle * M_PI / 180.0;

    // Start angle radial line
    int x1 = cx + (int)(inner_radius * cos(start_rad) + 0.5);
    int y1 = cy - (int)(inner_radius * sin(start_rad) + 0.5);
    int x2 = cx + (int)(radius * cos(start_rad) + 0.5);
    int y2 = cy - (int)(radius * sin(start_rad) + 0.5);
    graph_wline(g, x1, y1, x2, y2, rw, color);

    // End angle radial line
    x1 = cx + (int)(inner_radius * cos(end_rad) + 0.5);
    y1 = cy - (int)(inner_radius * sin(end_rad) + 0.5);
    x2 = cx + (int)(radius * cos(end_rad) + 0.5);
    y2 = cy - (int)(radius * sin(end_rad) + 0.5);
    graph_wline(g, x1, y1, x2, y2, rw, color);
}

/**
 * Draw a filled ring (annulus) arc with specified start angle, end angle, radius and thickness
 * Fills the entire ring area between inner and outer radius
 * @param g Graph context
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Outer radius of the ring
 * @param thickness Thickness of the ring (inner radius = radius - thickness)
 * @param start_angle Start angle in degrees (0-360, 0 is at 3 o'clock, counter-clockwise)
 * @param end_angle End angle in degrees (0-360)
 * @param color Ring color
 */
void graph_fill_ring_arc(graph_t* g, int cx, int cy, int radius, int thickness,
                    int start_angle, int end_angle, uint32_t color) {
    if (g == NULL || radius <= 0 || thickness <= 0 || thickness > radius)
        return;

    uint8_t fg_alpha = (color >> 24) & 0xFF;

    int inner_radius = radius - thickness;
    if (inner_radius < 0) inner_radius = 0;

    // Outer and inner circle radius squared
    int32_t outer_r_sq = radius * radius;
    int32_t inner_r_sq = inner_radius * inner_radius;

    // Anti-aliasing boundary (expanded to 1 pixel width for smoother effect)
    int32_t outer_aa_sq = outer_r_sq + 2 * radius;  // (r + 1)^2
    int32_t inner_aa_sq = (inner_radius > 0) ? inner_r_sq - 2 * inner_radius + 1 : 0;  // (inner_r - 1)^2
    if (inner_aa_sq < 0) inner_aa_sq = 0;

    // Calculate bounding box (including anti-aliasing area)
    int min_x = cx - radius - 1;
    int max_x = cx + radius + 1;
    int min_y = cy - radius - 1;
    int max_y = cy + radius + 1;

    if (min_x < 0) min_x = 0;
    if (min_y < 0) min_y = 0;
    if (max_x >= g->w) max_x = g->w - 1;
    if (max_y >= g->h) max_y = g->h - 1;

    // Angle normalization
    float start_rad = start_angle * M_PI / 180.0;
    float end_rad = end_angle * M_PI / 180.0;

    int swap = 0;
    if (start_angle > end_angle) {
        swap = 1;
    }

    // Scan pixel by pixel
    for (int y = min_y; y <= max_y; y++) {
        int32_t dy = y - cy;
        int32_t dy_sq = dy * dy;

        for (int x = min_x; x <= max_x; x++) {
            int32_t dx = x - cx;
            int32_t dist_sq = dx * dx + dy_sq;

            // Completely outside anti-aliasing area, skip
            if (dist_sq > outer_aa_sq) continue;

            // Completely inside inner circle (hollow part), skip
            if (dist_sq < inner_aa_sq) continue;

            // Check angle range
            float angle = atan2f(-dy, dx);
            int in_range = 0;
            
            // Normalize angle to [0, 2*PI)
            while (angle < 0) angle += 2 * M_PI;
            while (angle >= 2 * M_PI) angle -= 2 * M_PI;
            
            if (swap) {
                // Swap case: range is start_rad to 2*PI or 0 to end_rad
                if (angle >= start_rad || angle <= end_rad) {
                    in_range = 1;
                }
            } else {
                // Normal case: range is start_rad to end_rad
                if (angle >= start_rad && angle <= end_rad) {
                    in_range = 1;
                }
            }
            
            if (!in_range) continue;

            uint8_t alpha = 0;

            // Outer edge anti-aliasing area
            if (dist_sq >= outer_r_sq && dist_sq <= outer_aa_sq) {
                int32_t range = outer_aa_sq - outer_r_sq;
                int32_t dist_from_outer = outer_aa_sq - dist_sq;
                // Use square function for smoother transition
                int32_t t = (dist_from_outer * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // Inner edge anti-aliasing area
            else if (dist_sq >= inner_aa_sq && dist_sq <= inner_r_sq) {
                int32_t range = inner_r_sq - inner_aa_sq;
                int32_t dist_from_inner = dist_sq - inner_aa_sq;
                // Use square function for smoother transition
                int32_t t = (dist_from_inner * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256);
            }
            // Inside ring (completely between inner and outer circles)
            else if (dist_sq > inner_r_sq && dist_sq < outer_r_sq) {
                alpha = fg_alpha;
            }

            if (alpha > 0) {
                if (alpha >= 255) {
                    graph_pixel(g, x, y, (color & 0x00FFFFFF) | 0xFF000000);
                } else {
                    uint8_t r = (color >> 16) & 0xFF;
                    uint8_t gc = (color >> 8) & 0xFF;
                    uint8_t b = color & 0xFF;
                    graph_pixel_argb(g, x, y, alpha, r, gc, b);
                }
            }
        }
    }
}

/**
 * Draw a complete ring (full 360 degrees)
 * @param g Graph context
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Outer radius of the ring
 * @param thickness Thickness of the ring
 * @param color Ring color
 */
void graph_ring(graph_t* g, int cx, int cy, int radius, int thickness, int rw, uint32_t color) {
    graph_ring_arc(g, cx, cy, radius, thickness, rw, 0, 360, color);
}

/**
 * Draw a filled ring sector (pie chart style)
 * @param g Graph context
 * @param cx Center X coordinate
 * @param cy Center Y coordinate
 * @param radius Outer radius
 * @param thickness Thickness of the ring
 * @param start_angle Start angle in degrees
 * @param end_angle End angle in degrees
 * @param color Ring color
 */
void graph_ring_sector(graph_t* g, int cx, int cy, int radius, int thickness,
                       int start_angle, int end_angle, uint32_t color) {
    graph_fill_ring_arc(g, cx, cy, radius, thickness, start_angle, end_angle, color);
}
