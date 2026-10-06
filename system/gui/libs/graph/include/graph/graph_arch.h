#ifndef BSP_GRAPH_H
#define BSP_GRAPH_H

#include <graph/graph.h>

#ifdef __cplusplus 
extern "C" { 
#endif

/* All arch hooks return int: 0 on success, -1 on error (invalid arguments).
   Benign no-ops (e.g. a region fully clipped away) still report 0. */
int  graph_fill_arch(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
int  graph_blt_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
int  graph_blt_alpha_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);	
int  graph_blt_alpha_mask_arch(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);	

int  graph_scale_tof_arch(graph_t* g, graph_t* dst, double scale);
int  graph_scale_tof_fast_arch(graph_t* g, graph_t* dst, double scale);

int graph_rotate_to_arch(graph_t* g, graph_t* ret, int rot);

int  graph_gaussian_blur_arch(graph_t* g, int x, int y, int w, int h, int r);

int argb_2_nv12_arch(uint8_t  *out,  uint32_t *in , int w, int h);

int argb_2_rgb15_arch(uint16_t  *out,  uint32_t *in , int w, int h);
int rgb15_2_argb_arch(uint32_t  *out,  uint16_t *in , int w, int h);

int argb_2_rgb24_arch(uint32_t  *out,  uint32_t *in , int w, int h);
int rgb24_2_argb_arch(uint32_t  *out,  uint32_t *in , int w, int h);

int rgb15be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h);
int rgb24be_2_argb_arch(uint32_t *out, const uint8_t *in, int bpr, int w, int h);

#ifdef __cplusplus 
}
#endif

#endif