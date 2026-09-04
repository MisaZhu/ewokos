#ifndef GRAPH_G2D_H
#define GRAPH_G2D_H

#include <graph/graph.h>

#define G2D_MIN_SIZE (64*64)

#ifdef __cplusplus 
extern "C" { 
#endif

int   graph_g2d_avaliable(graph_t* g);

/* diagnostic counters for requests the device path turned down, and the
   pixels the caller then had to move itself. num = total rejects,
   noncontig = a shm canvas without contig backing (the silent-degradation
   case), small = below G2D_MIN_SIZE (expected). All pointers optional. */
void  graph_g2d_reject_stats(uint32_t* num, uint32_t* noncontig,
					uint32_t* small, uint64_t* pixels);

int   graph_fill_g2d(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
int   graph_blt_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
int   graph_blt_alpha_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);	

int   graph_scale_tof_g2d(graph_t* g, graph_t* dst, double scale);

int   graph_rotate_to_g2d(graph_t* g, graph_t* ret, int rot);

#ifdef __cplusplus 
}
#endif


#endif