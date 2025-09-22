#ifndef BSP_GRAPH_H
#define BSP_GRAPH_H

#include <graph/graph.h>

#ifdef __cplusplus 
extern "C" { 
#endif

void  graph_fill_bsp(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void  graph_blt_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void  graph_blt_alpha_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);	

void  graph_scale_tof_bsp(graph_t* g, graph_t* dst, double scale);

void  graph_glass_bsp(graph_t* g, int x, int y, int w, int h, int r);
void  graph_gaussian_bsp(graph_t* g, int x, int y, int w, int h, int r);

#ifdef __cplusplus 
}
#endif

#endif