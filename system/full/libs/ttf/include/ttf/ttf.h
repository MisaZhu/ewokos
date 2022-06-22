#ifndef TTF_H
#define TTF_H

#include <ttf/truety.h>
#include <graph/graph.h>
#include <hashmap.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	TTY_Font font;
	TTY_Instance inst;
	map_t *cache;
} ttf_font_t;

ttf_font_t* ttf_font_load(const char* fname, uint16_t ppm);

void ttf_font_free(ttf_font_t* font);

int  ttf_font_resize(ttf_font_t* font, uint16_t ppm);

void ttf_char_size(uint16_t c, ttf_font_t* font, uint16_t *w, uint16_t* h);
void ttf_text_size(const char* str, ttf_font_t* font, uint16_t margin, uint32_t *w, uint32_t* h);

int  ttf_font_hight(ttf_font_t* font);

int  ttf_font_width(ttf_font_t* font);

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h);
void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_font_t* font, uint16_t margin, uint32_t color);


#ifdef __cplusplus
}
#endif

#endif