#include <graph/graph.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#ifdef __cplusplus 
extern "C" { 
#endif

// Helper function: draw anti-aliased pixel (using integer alpha)
static inline void draw_aa_pixel_int(graph_t* g, int32_t x, int32_t y, uint32_t color, uint8_t alpha) {
    if (alpha <= 0) return;
    
    if (alpha >= 255) {
        graph_pixel(g, x, y, color);
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
    
    // Integer radius squared
    int32_t r_sq = radius * radius;
    // Anti-aliasing boundary radius: (radius + 0.5)^2 = radius^2 + radius + 0.25
    // Using integer: r_aa_sq = radius^2 + radius + 1 (ceiling)
    int32_t r_aa_sq = r_sq + radius + 1;
    // Inner threshold: (radius - 0.5)^2 = radius^2 - radius + 0.25
    int32_t r_inner_sq = r_sq - radius + 1;
    
    // Calculate bounding box
    int32_t min_y = cy - radius - 1;
    int32_t max_y = cy + radius + 1;
    int32_t min_x = cx - radius - 1;
    int32_t max_x = cx + radius + 1;
    
    // Clip to canvas bounds
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;
    
    // Scan line by line
    for (int32_t py = min_y; py <= max_y; py++) {
        int32_t dy = py - cy;
        int32_t dy_sq = dy * dy;
        
        // Calculate x range for this line (considering anti-aliasing area)
        // x^2 <= r_aa_sq - dy^2
        int32_t x_range_sq = r_aa_sq - dy_sq;
        if (x_range_sq < 0) continue;
        
        int32_t x_range = 0;
        // Integer square root approximation
        while (x_range * x_range <= x_range_sq && x_range <= radius + 1) {
            x_range++;
        }
        x_range--;
        
        int32_t row_min_x = cx - x_range;
        int32_t row_max_x = cx + x_range;
        
        if (row_min_x < min_x) row_min_x = min_x;
        if (row_max_x > max_x) row_max_x = max_x;
        
        // Calculate inner x range (no anti-aliasing needed)
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
        
        // Draw left boundary (needs anti-aliasing)
        for (int32_t px = row_min_x; px < inner_min_x; px++) {
            int32_t dx = px - cx;
            int32_t dist_sq = dx * dx + dy_sq;
            
            // Calculate anti-aliasing alpha (integer arithmetic)
            // dist = sqrt(dist_sq), edge_dist = dist - radius
            // intensity = 0.5 - edge_dist = 0.5 - (dist - radius) = radius + 0.5 - dist
            // alpha = fg_alpha * intensity
            
            // Using integer approximation:
            // When dist_sq is between r_inner_sq and r_aa_sq, anti-aliasing is needed
            if (dist_sq >= r_inner_sq && dist_sq <= r_aa_sq) {
                // Linear interpolation to calculate alpha
                // intensity = (r_aa_sq - dist_sq) / (r_aa_sq - r_inner_sq) approximation
                int32_t range = r_aa_sq - r_inner_sq;
                int32_t dist_from_outer = r_aa_sq - dist_sq;
                uint8_t alpha = (uint8_t)((fg_alpha * dist_from_outer + range / 2) / range);
                if (alpha > 0) {
                    draw_aa_pixel_int(g, px, py, color, alpha);
                }
            }
        }
        
        // Determine if this line is at top/bottom edge (needs y-direction anti-aliasing)
        // When dy_sq > r_inner_sq, it means in top/bottom edge area
        int is_top_bottom_edge = (dy_sq > r_inner_sq);
        
        // Draw inner circle
        if (is_top_bottom_edge) {
            // Top/bottom edge area: entire line needs anti-aliasing
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
            // Middle area: no anti-aliasing needed
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
        
        // Draw right boundary (needs anti-aliasing)
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

    uint8_t fg_alpha = (color >> 24) & 0xFF;

    // Outer and inner circle radius squared
    int32_t outer_r_sq = radius * radius;
    int32_t inner_r = radius - rw;
    int32_t inner_r_sq = inner_r * inner_r;

    // Anti-aliasing boundary (expanded to 1 pixel width for smoother effect)
    int32_t outer_aa_sq = outer_r_sq + 2 * radius;  // (r + 1)^2
    int32_t inner_aa_sq = (inner_r > 0) ? inner_r_sq - 2 * inner_r + 1 : 0;  // (inner_r - 1)^2
    if (inner_r <= 0) inner_aa_sq = 0;

    // Calculate bounding box
    int32_t min_y = cy - radius - 1;
    int32_t max_y = cy + radius + 1;
    int32_t min_x = cx - radius - 1;
    int32_t max_x = cx + radius + 1;

    // Clip to canvas bounds
    if (min_y < 0) min_y = 0;
    if (max_y >= g->h) max_y = g->h - 1;
    if (min_x < 0) min_x = 0;
    if (max_x >= g->w) max_x = g->w - 1;

    // Scan pixel by pixel (non-floating point, unified handling of all edges)
    for (int32_t py = min_y; py <= max_y; py++) {
        int32_t dy = py - cy;
        int32_t dy_sq = dy * dy;

        for (int32_t px = min_x; px <= max_x; px++) {
            int32_t dx = px - cx;
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
                draw_aa_pixel_int(g, px, py, color, alpha);
            }
        }
    }
}

#ifdef __cplusplus
}
#endif
