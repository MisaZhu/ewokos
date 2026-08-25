#include <graph/graph.h>

#ifdef ARCH_BOOST
#include <graph/graph_arch.h>
#endif

#include <graph/graph_g2d.h>

#ifdef __cplusplus 
extern "C" { 
#endif

static inline void rotate_90_clockwise_cache_optimized(const uint32_t* src, uint32_t* dst, 
                                       int width, int height) {
    for (int y = 0; y < height; y++) {
        const uint32_t* src_row = src + y * width;
        uint32_t* dst_col = dst + (height - 1 - y);
        
        for (int x = 0; x < width; x++) {
            *dst_col = src_row[x];
            dst_col += height;
        }
    }
}

static inline void rotate_90_counter_clockwise_cache_optimized(const uint32_t* src, uint32_t* dst, 
                                       int width, int height) {
    for (int y = 0; y < height; y++) {
        const uint32_t* src_row = src + y * width;
        uint32_t* dst_col = dst + y;
        
        for (int x = 0; x < width; x++) {
            *dst_col = src_row[(width - 1 - x)];
            dst_col += height;
        }
    }
}

void graph_rotate_to_cpu(graph_t* g, graph_t* ret, int rot) {
    if(g == NULL || ret == NULL)
        return;

    if(rot == G_ROTATE_90) {
        rotate_90_clockwise_cache_optimized(g->buffer, ret->buffer, g->w, g->h);
    }
    else if(rot == G_ROTATE_270) {
        rotate_90_counter_clockwise_cache_optimized(g->buffer, ret->buffer, g->w, g->h);
    }
    else if(rot == G_ROTATE_180) {
        int w0 = -(g->w);
        int w1 = ((g->h+1) * g->w) - 1;
        for(int i=0; i<g->h; ++i) {
            w0 += g->w;
            w1 -= g->w;
            for(int j=0; j<g->w; ++j) {
                ret->buffer[w0 + j] = g->buffer[w1 - j];
            }
        }
    }
}

void graph_rotate_to(graph_t* g, graph_t* ret, int rot) {
    if(graph_has_g2d() == 0 && g->shm_id > 0) {
        graph_rotate_to_g2d(g, ret, rot);
        return;
    }

#ifdef ARCH_BOOST
    graph_rotate_to_arch(g, ret, rot);
#else
    graph_rotate_to_cpu(g, ret, rot);
#endif
}

inline graph_t* graph_rotate(graph_t* g, int rot) {
    if(g == NULL)
        return NULL;
    graph_t* ret = NULL;

    if(rot == G_ROTATE_90 || rot == G_ROTATE_270) {
        ret = graph_new(NULL, g->h, g->w);
    }
    else if(rot == G_ROTATE_180) {
        ret = graph_new(NULL, g->w, g->h);
    }
    else 
        return NULL;
    graph_rotate_to(g, ret, rot);
    return ret;
}

#ifdef __cplusplus
}
#endif
