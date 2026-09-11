#ifndef GRAPH_CURVE_H
#define GRAPH_CURVE_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Quadratic Bezier curve: P0 -> (control C) -> P1
 * Flattened by adaptive de Casteljau subdivision and rendered with
 * graph_line/graph_wline so that clipping and alpha blending behave the
 * same way as the straight-line primitives. */
void graph_quadratic_curve(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx, int32_t cy,
        int32_t x1, int32_t y1,
        uint32_t color);

void graph_quadratic_curve_w(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx, int32_t cy,
        int32_t x1, int32_t y1,
        int32_t w, uint32_t color);

/* Cubic Bezier curve: P0 -> (control C1, C2) -> P1 */
void graph_bezier_curve(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx1, int32_t cy1,
        int32_t cx2, int32_t cy2,
        int32_t x1, int32_t y1,
        uint32_t color);

void graph_bezier_curve_w(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx1, int32_t cy1,
        int32_t cx2, int32_t cy2,
        int32_t x1, int32_t y1,
        int32_t w, uint32_t color);

/* Flatten-only variants: run the same adaptive de Casteljau subdivision the
 * stroking functions use, but write the resulting polyline into a caller
 * buffer instead of drawing it. `xy` receives interleaved x,y pairs (2 floats
 * per vertex) and must hold at least 2*max_pts floats. The start point
 * (x0,y0) is NOT written — it is the caller's pen position — so the returned
 * vertices are the curve's interior + end points, matching curve_emit's
 * contract. Returns the number of vertices written (0 on bad args / OOM).
 *
 * These let a caller that needs the geometry itself (e.g. a scanline polygon
 * fill, or a hit-test) reuse the library's flattening instead of duplicating
 * the subdivision math. Coordinates are float so the caller can flatten in
 * whatever space it likes (user or device) and transform afterwards. */
int graph_flatten_quadratic(float x0, float y0,
        float cx, float cy,
        float x1, float y1,
        float* xy, int max_pts);

int graph_flatten_cubic(float x0, float y0,
        float cx1, float cy1,
        float cx2, float cy2,
        float x1, float y1,
        float* xy, int max_pts);

#ifdef __cplusplus
}
#endif

#endif
