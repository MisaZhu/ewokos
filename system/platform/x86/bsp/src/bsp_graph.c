#include <graph/bsp_graph.h>

#ifdef __cplusplus
extern "C" {
#endif

void graph_fill_cpu(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color);
void graph_blt_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void graph_blt_alpha_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha);
void graph_blt_mask_cpu(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh);
void graph_scale_tof_cpu(graph_t* g, graph_t* dst, float scale);
void graph_gaussian_cpu(graph_t* g, int x, int y, int w, int h, int r);
void graph_glass_cpu(graph_t* g, int x, int y, int w, int h, int r);

void graph_fill_bsp(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t color) {
	graph_fill_cpu(g, x, y, w, h, color);
}

void graph_blt_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	graph_blt_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
}

void graph_blt_alpha_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh, uint8_t alpha) {
	graph_blt_alpha_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh, alpha);
}

void graph_blt_alpha_mask_bsp(graph_t* src, int32_t sx, int32_t sy, int32_t sw, int32_t sh,
		graph_t* dst, int32_t dx, int32_t dy, int32_t dw, int32_t dh) {
	graph_blt_mask_cpu(src, sx, sy, sw, sh, dst, dx, dy, dw, dh);
}

void graph_scale_tof_bsp(graph_t* g, graph_t* dst, double scale) {
	graph_scale_tof_cpu(g, dst, (float)scale);
}

void graph_scale_tof_fast_bsp(graph_t* g, graph_t* dst, double scale) {
	graph_scale_tof_cpu(g, dst, (float)scale);
}

void graph_glass_bsp(graph_t* g, int x, int y, int w, int h, int r) {
	graph_glass_cpu(g, x, y, w, h, r);
}

void graph_gaussian_bsp(graph_t* g, int x, int y, int w, int h, int r) {
	graph_gaussian_cpu(g, x, y, w, h, r);
}

#ifdef __cplusplus
}
#endif
