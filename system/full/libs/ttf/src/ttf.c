#include <ttf/ttf.h>

#ifdef __cplusplus
extern "C" {
#endif

TTY_Error tty_render_glyph_to_existing_graph(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph, graph_t* g, TTY_U32 x, TTY_U32 y, uint32_t color);

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, char c,
		ttf_font_t* font, ttf_inst_t* inst, uint32_t color) {
	TTY_U32 glyphIdx;
	if (tty_get_glyph_index(font, c, &glyphIdx))
		return;

	TTY_Glyph glyph;
	if (tty_glyph_init(font, &glyph, glyphIdx))
		return;

	tty_render_glyph_to_existing_graph(font, inst, &glyph, g, x, y, color);
}

void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_font_t* font, ttf_inst_t* inst, uint32_t color) {
	if(g == NULL)
		return;
	int32_t ox = x;
	while(*str) {
		if(*str == '\n') {
			x = ox;
			y += inst->maxGlyphSize.y;
		}
		else {
			graph_draw_char_ttf(g, x, y, *str, font, inst, color);
			x += inst->maxGlyphSize.x;
		}
		str++;
	}
}

#ifdef __cplusplus
}
#endif