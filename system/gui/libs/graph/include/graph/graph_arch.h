#ifndef BSP_GRAPH_H
#define BSP_GRAPH_H

#include <graph/graph.h>

#ifdef __cplusplus 
extern "C" { 
#endif

void  graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void  graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void  graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);	
void  graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);	

void  graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale);
void  graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale);

void graph_rotate_to_arch(graph_t* g, graph_t* ret, int rot);

void  graph_glass_arch(graph_t* g, int x, int y, int w, int h, int r);

void  graph_gaussian_arch(graph_t* g, int x, int y, int w, int h, int r);

void argb_2_nv12_arch(uint8_t  *out,  uint32_t *in , int w, int h);

void argb_2_rgb15_arch(uint16_t  *out,  uint32_t *in , int w, int h);
void rgb15_2_argb_arch(uint32_t  *out,  uint16_t *in , int w, int h);

void argb_2_rgb24_arch(uint32_t  *out,  uint32_t *in , int w, int h);
void rgb24_2_argb_arch(uint32_t  *out,  uint32_t *in , int w, int h);

void rgb15be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h);
void rgb24be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h);

#ifdef __cplusplus 
}
#endif

#endif