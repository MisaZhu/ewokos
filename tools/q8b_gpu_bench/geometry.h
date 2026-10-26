#ifndef Q8B_BENCH_GEOMETRY_H
#define Q8B_BENCH_GEOMETRY_H
#include <stdint.h>
/* Host compiler reflection; offsets address the concatenated draw blob. */
struct geom_meta {
    uint32_t uniforms;
    int32_t mvp, viewport_x, vertices, viewport_r, stride, first, count;
    int32_t blend, depth_write, cull, tex_w, tex_h;
};
#endif
