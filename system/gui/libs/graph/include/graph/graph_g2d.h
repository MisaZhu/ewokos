#ifndef GRAPH_G2D_H
#define GRAPH_G2D_H

#include <graph/graph.h>

#define G2D_MIN_SIZE (128*128)

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

/* gaussian-blur routing counters: gpu = dispatched to /dev/g2d,
   fb_* = fell back to the caller's cpu pass with the reason:
   notsup (op marked unsupported), canvas (non-shm/contig canvas),
   rect (partial or below-G2D_MIN_SIZE rect, refused by design),
   tmp (scratch alloc failed),
   drv (driver refused an otherwise-eligible request). all optional. */
void  graph_g2d_blur_stats(uint32_t* gpu, uint32_t* fb_notsup,
					uint32_t* fb_canvas, uint32_t* fb_rect,
					uint32_t* fb_tmp, uint32_t* fb_drv);

/* every graph_*_g2d answers G2D_OK when the device did the work; any
   non-zero (g2dclient's G2D_ERR_FAILED / G2D_ERR_NOT_SUPPORTED, see
   g2dclient/g2dclient.h) means "not done" and the caller runs its own
   cpu/arch pass instead. an op the driver declined with
   G2D_ERR_NOT_SUPPORTED is remembered per op and never offloaded again;
   the other ops keep using the device. */
int   graph_fill_g2d(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
int   graph_blt_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
int   graph_blt_alpha_g2d(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
					graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);	

int   graph_scale_tof_g2d(graph_t* g, graph_t* dst, double scale);

int   graph_rotate_to_g2d(graph_t* g, graph_t* ret, int rot);

int   graph_gaussian_blur_g2d(graph_t* g, int x, int y, int w, int h, int r);

#ifdef __cplusplus 
}
#endif


#endif