#ifndef TTF_H
#define TTF_H

#include <ttf/truety.h>
#include <graph/graph.h>
#include <ewoksys/hashmap.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef TTY_Font ttf_t;

typedef struct {
	TTY_Instance inst;
} ttf_font_t;

ttf_t* ttf_load(const char* fname);
ttf_font_t*  ttf_new_inst(ttf_t* ttf, uint16_t ppm);

int ttf_render_glyph_cache(ttf_t* ttf, ttf_font_t* font, TTY_U32 c, TTY_Glyph* glyph);

void ttf_free(ttf_t* ttf);

void ttf_font_free(ttf_font_t* font);

int  ttf_font_resize(ttf_t* ttf, ttf_font_t* font, uint16_t ppm);

void ttf_char_size(ttf_t* ttf, ttf_font_t* font, uint16_t c, uint16_t *w, uint16_t* h); 
void ttf_text_size(ttf_t* ttf, ttf_font_t* font, const char* str, uint32_t *w, uint32_t* h);

int  ttf_font_hight(ttf_font_t* font);
int  ttf_font_width(ttf_font_t* font);

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_t* ttf, ttf_font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h);
void graph_draw_char_ttf_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_t* ttf, ttf_font_t* font, uint32_t color, TTY_U16 w, TTY_U16 h);
void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_t* ttf, ttf_font_t* font, uint32_t color);


#ifdef __cplusplus
}
#endif

#endif