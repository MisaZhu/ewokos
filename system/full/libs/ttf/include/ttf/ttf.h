#ifndef TTF_H
#define TTF_H

#include <ttf/truety.h>
#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef TTY_Font ttf_font_t; 
typedef TTY_Instance ttf_inst_t; 

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c, 
		ttf_font_t* font, ttf_inst_t* inst, uint32_t color, TTY_U16* w, TTY_U16* h);
void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_font_t* font, ttf_inst_t* inst, uint16_t margin, uint32_t color);

#ifdef __cplusplus
}
#endif

#endif