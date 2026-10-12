#include <graph/graph.h>
#include <graph/graph_arch.h>
#include <graph/graph_g2d.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus 
extern "C" { 
#endif

/* ------------------------------------------------------------------ */
/* filled triangle (cpu reference)                                     */
/* ------------------------------------------------------------------ */

/* software scanline fill, the portable reference for graph_fill_triangle.
   sort the three vertices by ascending Y, walk the two edge slopes per
   row and round each span exactly like the arch/g2d rasterizers (same
   iround() convention), then plot every pixel through graph_pixel, which
   clips to g->clip / the buffer bounds and does the opaque-store or the
   source-over blend. a fully transparent color or a flat (zero-height)
   triangle is a no-op. the span geometry matches arch_g2d_fill_triangle;
   the per-pixel blend may round +/-1 against the SIMD span fill, exactly
   as graph_fill_cpu does against graph_fill_arch. */
void graph_fill_triangle_cpu(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        int32_t x3, int32_t y3, uint32_t color) {
    int32_t x0i = x1, y0i = y1, x1i = x2, y1i = y2, x2i = x3, y2i = y3;
    int32_t t, y, ymin, ymax;
    float s01, s02, s12;

    if(g == NULL || g->buffer == NULL)
        return;
    if(color_a(color) == 0)
        return;

    /* sort the three vertices by ascending Y */
    if(y0i > y1i) { t = x0i; x0i = x1i; x1i = t; t = y0i; y0i = y1i; y1i = t; }
    if(y0i > y2i) { t = x0i; x0i = x2i; x2i = t; t = y0i; y0i = y2i; y2i = t; }
    if(y1i > y2i) { t = x1i; x1i = x2i; x2i = t; t = y1i; y1i = y2i; y2i = t; }

    if(y2i == y0i)
        return; /* degenerate (flat) */

    s01 = (y1i > y0i) ? (float)(x1i - x0i) / (float)(y1i - y0i) : 0.0f;
    s02 = (float)(x2i - x0i) / (float)(y2i - y0i);
    s12 = (y2i > y1i) ? (float)(x2i - x1i) / (float)(y2i - y1i) : 0.0f;

    /* clamp the scan range to the canvas rows; graph_pixel clips x and
       applies g->clip per pixel */
    ymin = (y0i < 0) ? 0 : y0i;
    ymax = (y2i > g->h - 1) ? g->h - 1 : y2i;

    for(y = ymin; y <= ymax; y++) {
        float xl, xr, tt;
        int32_t xs, xe, x;

        /* left edge: v0->v1 while at/above v1, then v1->v2; a flat top
           spans v0..v1 on the shared row */
        if(y == y0i && y1i == y0i) {
            xl = (float)x1i;
        } else if(y <= y1i) {
            xl = (float)x0i + s01 * (float)(y - y0i);
        } else {
            xl = (float)x1i + s12 * (float)(y - y1i);
        }
        /* right edge: v0->v2 for the whole span */
        xr = (float)x0i + s02 * (float)(y - y0i);

        if(xl > xr) { tt = xl; xl = xr; xr = tt; }

        xs = (xl >= 0.0f) ? (int32_t)(xl + 0.5f) : (int32_t)(xl - 0.5f);
        xe = (xr >= 0.0f) ? (int32_t)(xr + 0.5f) : (int32_t)(xr - 0.5f);

        for(x = xs; x <= xe; x++)
            graph_pixel(g, x, y, color);
    }
}

/* ------------------------------------------------------------------ */
/* anti-aliased outline (cpu)                                          */
/* ------------------------------------------------------------------ */

static int32_t tri_iabs(int32_t v) { return (v < 0) ? -v : v; }

/* floor-based integer/fraction split, so negative coordinates anti-alias
   the same way positive ones do (plain (int)v truncation would bias the
   coverage toward zero). no libm: only float arithmetic. */
static int32_t tri_ipart(float v) {
    int32_t i = (int32_t)v;
    return ((float)i > v) ? i - 1 : i;
}
static float tri_fpart(float v) { return v - (float)tri_ipart(v); }
static float tri_rfpart(float v) { return 1.0f - tri_fpart(v); }

/* plot one coverage sample of an anti-aliased edge: brightness in [0,1]
   scales the color's own alpha, and graph_pixel_argb clips and blends. */
static void tri_plot(graph_t* g, int32_t x, int32_t y, float brightness,
        uint8_t ca, uint8_t cr, uint8_t cg, uint8_t cb) {
    int32_t a = (int32_t)((float)ca * brightness + 0.5f);
    if(a <= 0)
        return;
    if(a > 255)
        a = 255;
    graph_pixel_argb(g, x, y, (uint8_t)a, cr, cg, cb);
}

/* Xiaolin Wu's anti-aliased line, integer endpoints (so round() is the
   identity and the endpoint gap factor is the fixed 0.5). used for the
   three edges of graph_triangle. */
static void tri_aa_line(graph_t* g, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
        uint8_t ca, uint8_t cr, uint8_t cg, uint8_t cb) {
    int steep = tri_iabs(y1 - y0) > tri_iabs(x1 - x0);
    int32_t t, dx, dy, xpxl1, xpxl2, x;
    float gradient, yend, intery;

    if(steep) {
        t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }
    if(x0 > x1) {
        t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }

    dx = x1 - x0;
    dy = y1 - y0;
    gradient = (dx == 0) ? 1.0f : (float)dy / (float)dx;

    /* first endpoint (integer x0: xgap = rfpart(x0+0.5) = 0.5) */
    yend = (float)y0;
    xpxl1 = x0;
    if(steep) {
        tri_plot(g, tri_ipart(yend), xpxl1, tri_rfpart(yend) * 0.5f, ca, cr, cg, cb);
        tri_plot(g, tri_ipart(yend) + 1, xpxl1, tri_fpart(yend) * 0.5f, ca, cr, cg, cb);
    } else {
        tri_plot(g, xpxl1, tri_ipart(yend), tri_rfpart(yend) * 0.5f, ca, cr, cg, cb);
        tri_plot(g, xpxl1, tri_ipart(yend) + 1, tri_fpart(yend) * 0.5f, ca, cr, cg, cb);
    }
    intery = yend + gradient;

    /* second endpoint (integer x1: xgap = fpart(x1+0.5) = 0.5) */
    yend = (float)y1;
    xpxl2 = x1;
    if(steep) {
        tri_plot(g, tri_ipart(yend), xpxl2, tri_rfpart(yend) * 0.5f, ca, cr, cg, cb);
        tri_plot(g, tri_ipart(yend) + 1, xpxl2, tri_fpart(yend) * 0.5f, ca, cr, cg, cb);
    } else {
        tri_plot(g, xpxl2, tri_ipart(yend), tri_rfpart(yend) * 0.5f, ca, cr, cg, cb);
        tri_plot(g, xpxl2, tri_ipart(yend) + 1, tri_fpart(yend) * 0.5f, ca, cr, cg, cb);
    }

    /* main walk between the two endpoints */
    if(steep) {
        for(x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
            tri_plot(g, tri_ipart(intery), x, tri_rfpart(intery), ca, cr, cg, cb);
            tri_plot(g, tri_ipart(intery) + 1, x, tri_fpart(intery), ca, cr, cg, cb);
            intery += gradient;
        }
    } else {
        for(x = xpxl1 + 1; x <= xpxl2 - 1; x++) {
            tri_plot(g, x, tri_ipart(intery), tri_rfpart(intery), ca, cr, cg, cb);
            tri_plot(g, x, tri_ipart(intery) + 1, tri_fpart(intery), ca, cr, cg, cb);
            intery += gradient;
        }
    }
}

/* anti-aliased triangle outline: three Wu edges. there is no device or
   arch outline primitive, so graph_triangle always lands here. */
void graph_triangle_cpu(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        int32_t x3, int32_t y3, uint32_t color) {
    uint8_t ca, cr, cg, cb;

    if(g == NULL || g->buffer == NULL)
        return;
    ca = (color >> 24) & 0xff;
    if(ca == 0)
        return;
    cr = (color >> 16) & 0xff;
    cg = (color >> 8) & 0xff;
    cb = color & 0xff;

    tri_aa_line(g, x1, y1, x2, y2, ca, cr, cg, cb);
    tri_aa_line(g, x2, y2, x3, y3, ca, cr, cg, cb);
    tri_aa_line(g, x3, y3, x1, y1, ca, cr, cg, cb);
}

/* ------------------------------------------------------------------ */
/* dispatchers                                                         */
/* ------------------------------------------------------------------ */

/* filled triangle: try the device offload (shm-backed canvases only; a
   partial clip or a driver without the triangle primitive declines), then
   the in-process arch rasterizer, then the portable cpu reference. */
void graph_fill_triangle(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        int32_t x3, int32_t y3, uint32_t color) {
    if(graph_g2d_avaliable(g) == 0) {
        if(graph_fill_triangle_g2d(g, x1, y1, x2, y2, x3, y3, color) == 0)
            return;
    }

#ifdef ARCH_BOOST
    if(graph_fill_triangle_arch(g, x1, y1, x2, y2, x3, y3, color) == 0)
        return;
#endif
    graph_fill_triangle_cpu(g, x1, y1, x2, y2, x3, y3, color);
}

/* triangle outline: the device and arch hooks both decline (no outline
   primitive), so this resolves to the anti-aliased cpu walk; the hooks are
   kept for symmetry with graph_fill_triangle and a future accelerated
   edge path. */
void graph_triangle(graph_t* g, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
        int32_t x3, int32_t y3, uint32_t color) {
    if(graph_g2d_avaliable(g) == 0) {
        if(graph_triangle_g2d(g, x1, y1, x2, y2, x3, y3, color) == 0)
            return;
    }

#ifdef ARCH_BOOST
    if(graph_triangle_arch(g, x1, y1, x2, y2, x3, y3, color) == 0)
        return;
#endif
    graph_triangle_cpu(g, x1, y1, x2, y2, x3, y3, color);
}

#ifdef __cplusplus
}
#endif
