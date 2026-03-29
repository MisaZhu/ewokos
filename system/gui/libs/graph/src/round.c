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

// 辅助函数：绘制圆角（按行扫描优化，非浮点实现）
// corner_x, corner_y: 圆角矩形区域的起始坐标
// cx, cy: 圆角中心相对于corner的偏移
// r: 圆角半径
static inline void draw_round_corner_fill(graph_t* g, int32_t corner_x, int32_t corner_y,
                                          int32_t cx, int32_t cy, int32_t r,
                                          uint32_t color, int mirror_x, int mirror_y) {
    uint8_t fg_alpha = (color >> 24) & 0xFF;

    // 整数半径平方
    int32_t r_sq = r * r;
    // 抗锯齿边界半径：(r + 0.5)^2 = r^2 + r + 0.25，使用整数向上取整
    int32_t r_aa_sq = r_sq + r + 1;
    // 内圈阈值：(r - 0.5)^2 = r^2 - r + 0.25
    int32_t r_inner_sq = r_sq - r + 1;

    // 计算边界框（相对于圆角中心），限制在 r-1 范围内以与四边对齐
    int32_t max_dy = r - 1;

    // 按行扫描
    for (int32_t dy = 0; dy <= max_dy; dy++) {
        int32_t dy_sq = dy * dy;

        // 计算这一行的x范围（考虑抗锯齿区域）
        int32_t x_range_sq = r_aa_sq - dy_sq;
        if (x_range_sq < 0) continue;

        // 整数平方根近似，限制在 r-1 范围内
        int32_t x_range = 0;
        while (x_range * x_range <= x_range_sq && x_range <= r - 1) {
            x_range++;
        }
        x_range--;
        if (x_range < 0) x_range = 0;

        // 计算内圈x范围（不需要抗锯齿的部分）
        int32_t inner_x_range_sq = r_inner_sq - dy_sq;
        int32_t inner_x_range = 0;
        if (inner_x_range_sq > 0) {
            while (inner_x_range * inner_x_range <= inner_x_range_sq && inner_x_range <= r - 1) {
                inner_x_range++;
            }
            inner_x_range--;
        }

        // 绘制这一行
        for (int32_t dx = 0; dx <= x_range; dx++) {
            int32_t dist_sq = dx * dx + dy_sq;

            // 计算像素位置
            int px = corner_x + cx + (mirror_x ? -dx : dx);
            int py = corner_y + cy + (mirror_y ? -dy : dy);

            // 检查是否在画布范围内
            if (px < 0 || px >= g->w || py < 0 || py >= g->h) continue;

            // 判断是否需要抗锯齿
            if (dist_sq >= r_inner_sq && dist_sq <= r_aa_sq) {
                // 抗锯齿边缘
                int32_t range = r_aa_sq - r_inner_sq;
                int32_t dist_from_outer = r_aa_sq - dist_sq;
                uint8_t alpha = (uint8_t)((fg_alpha * dist_from_outer + range / 2) / range);
                if (alpha > 0) {
                    draw_aa_pixel_int(g, px, py, color, alpha);
                }
            } else if (dist_sq < r_inner_sq) {
                // 内部实心区域
                if (fg_alpha >= 255) {
                    graph_pixel(g, px, py, color);
                } else if (fg_alpha > 0) {
                    uint8_t rc = (color >> 16) & 0xFF;
                    uint8_t gc = (color >> 8) & 0xFF;
                    uint8_t b = color & 0xFF;
                    graph_pixel_argb(g, px, py, fg_alpha, rc, gc, b);
                }
            }
        }
    }
}

// 辅助函数：绘制圆角边框（按行扫描优化，非浮点实现）
static inline void draw_round_corner_round(graph_t* g, int32_t corner_x, int32_t corner_y,
                                           int32_t cx, int32_t cy, int32_t r, int32_t rw,
                                           uint32_t color, int mirror_x, int mirror_y) {
    uint8_t fg_alpha = (color >> 24) & 0xFF;

    // 外圆和内圆的半径平方
    int32_t outer_r_sq = r * r;
    int32_t inner_r = r - rw;
    int32_t inner_r_sq = inner_r * inner_r;

    // 抗锯齿边界（扩大到1像素宽度以获得更平滑的效果）
    int32_t outer_aa_sq = outer_r_sq + 2 * r;  // (r + 1)^2
    int32_t inner_aa_sq = (inner_r > 0) ? inner_r_sq - 2 * inner_r + 1 : 0;  // (inner_r - 1)^2
    if (inner_r <= 0) inner_aa_sq = 0;

    // 计算边界框，限制在 r-1 范围内以与四边对齐
    int32_t max_dy = r - 1;

    // 按行扫描
    for (int32_t dy = 0; dy <= max_dy; dy++) {
        int32_t dy_sq = dy * dy;

        // 计算这一行在外圆内的x范围
        int32_t outer_x_sq = outer_aa_sq - dy_sq;
        if (outer_x_sq < 0) continue;

        int32_t max_dx = 0;
        while (max_dx * max_dx <= outer_x_sq && max_dx <= r - 1) {
            max_dx++;
        }
        max_dx--;
        if (max_dx < 0) max_dx = 0;

        // 计算这一行在内圆内的x范围
        int32_t inner_x_sq = inner_r_sq - dy_sq;
        int32_t inner_max_dx = 0;
        if (inner_x_sq > 0) {
            while (inner_max_dx * inner_max_dx <= inner_x_sq && inner_max_dx <= inner_r) {
                inner_max_dx++;
            }
            inner_max_dx--;
        }

        // 绘制这一行
        for (int32_t dx = 0; dx <= max_dx; dx++) {
            int32_t dist_sq = dx * dx + dy_sq;

            // 完全在抗锯齿区域外，跳过
            if (dist_sq > outer_aa_sq) continue;

            // 完全在内圆内部（空心部分），跳过
            if (dist_sq < inner_aa_sq) continue;

            // 计算像素位置
            int px = corner_x + cx + (mirror_x ? -dx : dx);
            int py = corner_y + cy + (mirror_y ? -dy : dy);

            // 检查是否在画布范围内
            if (px < 0 || px >= g->w || py < 0 || py >= g->h) continue;

            uint8_t alpha = 0;

            // 外边缘抗锯齿区域
            if (dist_sq >= outer_r_sq && dist_sq <= outer_aa_sq) {
                int32_t range = outer_aa_sq - outer_r_sq;
                int32_t dist_from_outer = outer_aa_sq - dist_sq;
                // 使用平方函数获得更平滑的过渡
                int32_t t = (dist_from_outer * 256) / range;
                int32_t smoothed = (t * t) / 256;
                alpha = (uint8_t)((fg_alpha * smoothed) / 256) / 2;
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
            // 圆环内部（完全在内外圆之间）
            else if (dist_sq > inner_r_sq && dist_sq < outer_r_sq) {
                alpha = fg_alpha;
            }

            if (alpha > 0) {
                draw_aa_pixel_int(g, px, py, color, alpha);
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

	// Draw four corners with anti-aliasing (按行扫描优化，非浮点)
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

	// Draw four corners with anti-aliasing (按行扫描优化，非浮点)
	// Top-left corner
	draw_round_corner_round(g, x, y, r-1, r-1, r, rw, color, 1, 1);

	// Top-right corner
	draw_round_corner_round(g, x+w-r, y, 0, r-1, r, rw, color, 0, 1);

	// Bottom-left corner
	draw_round_corner_round(g, x, y+h-r, r-1, 0, r, rw, color, 1, 0);

	// Bottom-right corner
	draw_round_corner_round(g, x+w-r, y+h-r, 0, 0, r, rw, color, 0, 0);
}

#ifdef __cplusplus
}
#endif
