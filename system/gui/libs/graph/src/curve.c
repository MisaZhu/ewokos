#include <graph/curve.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Flatness tolerance for adaptive subdivision. Chord tolerance ~0.25px;
   the tests below compare squared distances so no sqrt is needed. */
#define CURVE_TOL          0.25f
#define CURVE_TOL_SQ       (CURVE_TOL * CURVE_TOL)
/* Standard cubic flatness bound uses 16*tol^2 on max(u^2,v^2). */
#define CURVE_TOL_SQ_CUBIC (16.0f * CURVE_TOL * CURVE_TOL)

/* Guard against runaway recursion on pathological inputs. */
#define CURVE_QUAD_STACK_MAX   64
#define CURVE_CUBIC_STACK_MAX 128
/* Upper bound on the number of flattened segments we are willing to
   allocate for one curve. Sized for a ~2000px stroke at 0.25px chord
   tolerance with plenty of headroom. */
#define CURVE_SEGMENT_MAX      4096

typedef struct {
    float x, y;
} curve_pt_t;

/* Flatten a quadratic Bezier (x0,y0) -> (cx,cy) -> (x1,y1) into pts.
   pts[0] is not written (it is the caller-provided pen position);
   the function appends every subsequent vertex and returns the total
   number of vertices written. */
static int curve_flatten_quadratic(float x0, float y0,
                                   float cx, float cy,
                                   float x1, float y1,
                                   curve_pt_t* pts, int max_pts) {
    float stack[CURVE_QUAD_STACK_MAX][6];
    int sp = 0;
    int n = 0;

    stack[sp][0] = x0; stack[sp][1] = y0;
    stack[sp][2] = cx; stack[sp][3] = cy;
    stack[sp][4] = x1; stack[sp][5] = y1;
    sp++;

    while(sp > 0 && n < max_pts) {
        sp--;
        float ax = stack[sp][0], ay = stack[sp][1];
        float bx = stack[sp][2], by = stack[sp][3];
        float dx = stack[sp][4], dy = stack[sp][5];

        /* Distance from control point to the chord midpoint. */
        float mx = (ax + 2.0f * bx + dx) * 0.25f;
        float my = (ay + 2.0f * by + dy) * 0.25f;
        float ex = bx - mx, ey = by - my;
        float dist_sq = ex * ex + ey * ey;

        if(dist_sq <= CURVE_TOL_SQ || sp + 2 >= CURVE_QUAD_STACK_MAX) {
            pts[n].x = dx;
            pts[n].y = dy;
            n++;
        }
        else {
            /* de Casteljau split at t=0.5. */
            float p1x = (ax + bx) * 0.5f, p1y = (ay + by) * 0.5f;
            float p2x = (bx + dx) * 0.5f, p2y = (by + dy) * 0.5f;
            float p3x = (p1x + p2x) * 0.5f, p3y = (p1y + p2y) * 0.5f;

            stack[sp][0] = ax;   stack[sp][1] = ay;
            stack[sp][2] = p1x;  stack[sp][3] = p1y;
            stack[sp][4] = p3x;  stack[sp][5] = p3y;
            sp++;
            stack[sp][0] = p3x;  stack[sp][1] = p3y;
            stack[sp][2] = p2x;  stack[sp][3] = p2y;
            stack[sp][4] = dx;   stack[sp][5] = dy;
            sp++;
        }
    }
    return n;
}

/* Flatten a cubic Bezier (x0,y0) -> (c1x,c1y) -> (c2x,c2y) -> (x1,y1)
   into pts, same contract as the quadratic version. */
static int curve_flatten_cubic(float x0, float y0,
                               float c1x, float c1y,
                               float c2x, float c2y,
                               float x1, float y1,
                               curve_pt_t* pts, int max_pts) {
    float stack[CURVE_CUBIC_STACK_MAX][8];
    int sp = 0;
    int n = 0;

    stack[sp][0] = x0;  stack[sp][1] = y0;
    stack[sp][2] = c1x; stack[sp][3] = c1y;
    stack[sp][4] = c2x; stack[sp][5] = c2y;
    stack[sp][6] = x1;  stack[sp][7] = y1;
    sp++;

    while(sp > 0 && n < max_pts) {
        sp--;
        float ax  = stack[sp][0], ay  = stack[sp][1];
        float bx  = stack[sp][2], by  = stack[sp][3];
        float cx2 = stack[sp][4], cy2 = stack[sp][5];
        float dx  = stack[sp][6], dy  = stack[sp][7];

        /* Standard flatness bound: max of the two control-point
           deviations from the chord, squared. */
        float ux = 3.0f * bx  - 2.0f * ax - dx; ux *= ux;
        float uy = 3.0f * by  - 2.0f * ay - dy; uy *= uy;
        float vx = 3.0f * cx2 - 2.0f * dx - ax; vx *= vx;
        float vy = 3.0f * cy2 - 2.0f * dy - ay; vy *= vy;
        if(ux < vx) ux = vx;
        if(uy < vy) uy = vy;

        if((ux + uy) <= CURVE_TOL_SQ_CUBIC || sp + 2 >= CURVE_CUBIC_STACK_MAX) {
            pts[n].x = dx;
            pts[n].y = dy;
            n++;
        }
        else {
            /* de Casteljau split at t=0.5. */
            float p1x = (ax + bx) * 0.5f,    p1y = (ay + by) * 0.5f;
            float p2x = (bx + cx2) * 0.5f,   p2y = (by + cy2) * 0.5f;
            float p3x = (cx2 + dx) * 0.5f,   p3y = (cy2 + dy) * 0.5f;
            float p4x = (p1x + p2x) * 0.5f,  p4y = (p1y + p2y) * 0.5f;
            float p5x = (p2x + p3x) * 0.5f,  p5y = (p2y + p3y) * 0.5f;
            float p6x = (p4x + p5x) * 0.5f,  p6y = (p4y + p5y) * 0.5f;

            stack[sp][0] = ax;  stack[sp][1] = ay;
            stack[sp][2] = p1x; stack[sp][3] = p1y;
            stack[sp][4] = p4x; stack[sp][5] = p4y;
            stack[sp][6] = p6x; stack[sp][7] = p6y;
            sp++;
            stack[sp][0] = p6x; stack[sp][1] = p6y;
            stack[sp][2] = p5x; stack[sp][3] = p5y;
            stack[sp][4] = p3x; stack[sp][5] = p3y;
            stack[sp][6] = dx;  stack[sp][7] = dy;
            sp++;
        }
    }
    return n;
}

/* Emit the flattened polyline. width==0 uses graph_line (1px), otherwise
   graph_wline is used so callers get the same look as straight strokes. */
static void curve_emit(graph_t* g, int32_t x0, int32_t y0,
                       const curve_pt_t* pts, int n,
                       int32_t width, uint32_t color) {
    int32_t px = x0;
    int32_t py = y0;

    for(int i = 0; i < n; i++) {
        int32_t nx = (int32_t)(pts[i].x + (pts[i].x >= 0.0f ? 0.5f : -0.5f));
        int32_t ny = (int32_t)(pts[i].y + (pts[i].y >= 0.0f ? 0.5f : -0.5f));
        if(width > 1)
            graph_wline(g, px, py, nx, ny, (uint32_t)width, color);
        else
            graph_line(g, px, py, nx, ny, color);
        px = nx;
        py = ny;
    }
}

void graph_quadratic_curve(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx, int32_t cy,
        int32_t x1, int32_t y1,
        uint32_t color) {
    if(g == NULL)
        return;

    curve_pt_t* pts = (curve_pt_t*)malloc(sizeof(curve_pt_t) * CURVE_SEGMENT_MAX);
    if(pts == NULL)
        return;
    int n = curve_flatten_quadratic((float)x0, (float)y0,
                                    (float)cx, (float)cy,
                                    (float)x1, (float)y1,
                                    pts, CURVE_SEGMENT_MAX);
    curve_emit(g, x0, y0, pts, n, 1, color);
    free(pts);
}

void graph_quadratic_curve_w(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx, int32_t cy,
        int32_t x1, int32_t y1,
        int32_t w, uint32_t color) {
    if(g == NULL || w <= 0)
        return;

    curve_pt_t* pts = (curve_pt_t*)malloc(sizeof(curve_pt_t) * CURVE_SEGMENT_MAX);
    if(pts == NULL)
        return;
    int n = curve_flatten_quadratic((float)x0, (float)y0,
                                    (float)cx, (float)cy,
                                    (float)x1, (float)y1,
                                    pts, CURVE_SEGMENT_MAX);
    curve_emit(g, x0, y0, pts, n, w, color);
    free(pts);
}

void graph_bezier_curve(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx1, int32_t cy1,
        int32_t cx2, int32_t cy2,
        int32_t x1, int32_t y1,
        uint32_t color) {
    if(g == NULL)
        return;

    curve_pt_t* pts = (curve_pt_t*)malloc(sizeof(curve_pt_t) * CURVE_SEGMENT_MAX);
    if(pts == NULL)
        return;
    int n = curve_flatten_cubic((float)x0, (float)y0,
                                (float)cx1, (float)cy1,
                                (float)cx2, (float)cy2,
                                (float)x1, (float)y1,
                                pts, CURVE_SEGMENT_MAX);
    curve_emit(g, x0, y0, pts, n, 1, color);
    free(pts);
}

void graph_bezier_curve_w(graph_t* g,
        int32_t x0, int32_t y0,
        int32_t cx1, int32_t cy1,
        int32_t cx2, int32_t cy2,
        int32_t x1, int32_t y1,
        int32_t w, uint32_t color) {
    if(g == NULL || w <= 0)
        return;

    curve_pt_t* pts = (curve_pt_t*)malloc(sizeof(curve_pt_t) * CURVE_SEGMENT_MAX);
    if(pts == NULL)
        return;
    int n = curve_flatten_cubic((float)x0, (float)y0,
                                (float)cx1, (float)cy1,
                                (float)cx2, (float)cy2,
                                (float)x1, (float)y1,
                                pts, CURVE_SEGMENT_MAX);
    curve_emit(g, x0, y0, pts, n, w, color);
    free(pts);
}

/* Flatten-only entry points. curve_pt_t is exactly two floats {x,y}, so the
 * vertex array the internal flatteners fill is copied out as interleaved x,y
 * pairs. Same subdivision, no drawing — lets callers reuse the library's
 * flattening for fills/hit-tests instead of duplicating the math. */
int graph_flatten_quadratic(float x0, float y0,
        float cx, float cy,
        float x1, float y1,
        float* xy, int max_pts) {
    if(xy == NULL || max_pts <= 0)
        return 0;

    curve_pt_t* pts = (curve_pt_t*)malloc(sizeof(curve_pt_t) * max_pts);
    if(pts == NULL)
        return 0;
    int n = curve_flatten_quadratic(x0, y0, cx, cy, x1, y1, pts, max_pts);
    for(int i = 0; i < n; i++) {
        xy[2 * i]     = pts[i].x;
        xy[2 * i + 1] = pts[i].y;
    }
    free(pts);
    return n;
}

int graph_flatten_cubic(float x0, float y0,
        float cx1, float cy1,
        float cx2, float cy2,
        float x1, float y1,
        float* xy, int max_pts) {
    if(xy == NULL || max_pts <= 0)
        return 0;

    curve_pt_t* pts = (curve_pt_t*)malloc(sizeof(curve_pt_t) * max_pts);
    if(pts == NULL)
        return 0;
    int n = curve_flatten_cubic(x0, y0, cx1, cy1, cx2, cy2, x1, y1, pts, max_pts);
    for(int i = 0; i < n; i++) {
        xy[2 * i]     = pts[i].x;
        xy[2 * i + 1] = pts[i].y;
    }
    free(pts);
    return n;
}

#ifdef __cplusplus
}
#endif
