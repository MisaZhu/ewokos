#ifndef GRAPH_EX_H
#define GRAPH_EX_H

#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

void graph_get_3d_color(uint32_t base, uint32_t *dark, uint32_t *bright);
void graph_box_3d(graph_t* g, int x, int y, int w, int h, uint32_t bright_color, uint32_t dark_color);
void graph_frame(graph_t* g, int x, int y, int w, int h, int width, uint32_t base_color, bool rev);
void graph_fill_3d(graph_t* g, int x, int y, int w, int h, uint32_t color, bool rev);
void graph_draw_dot_pattern(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2, uint8_t dspace, uint8_t dw);
void graph_gradation(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2, bool vertical);
void graph_glass_cpu(graph_t* g,int x, int y, int w, int h, int r);
void graph_glass(graph_t* g,int x, int y, int w, int h, int r);
void graph_gaussian_cpu(graph_t* g,int x, int y, int w, int h, int r);
void graph_gaussian(graph_t* g,int x, int y, int w, int h, int r);
void graph_shadow(graph_t* g, int x, int y, int w, int h, uint8_t shadow, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif
