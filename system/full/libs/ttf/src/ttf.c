#include <ttf/ttf.h>
#include <sys/utf8unicode.h>

#ifdef __cplusplus
extern "C" {
#endif

TTY_Error tty_render_glyph_to_existing_graph(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph, graph_t* g, TTY_U32 x, TTY_U32 y, uint32_t color);

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_font_t* font, ttf_inst_t* inst, uint32_t color, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_U32 glyphIdx;
	if (tty_get_glyph_index(font, c, &glyphIdx))
		return;

	TTY_Glyph glyph;
	if (tty_glyph_init(font, &glyph, glyphIdx))
		return;

	tty_render_glyph_to_existing_graph(font, inst, &glyph, g, x, y, color);
	if(w != NULL)
		*w = glyph.size.x == 0 ?  inst->maxGlyphSize.x : glyph.size.x;

	if(h != NULL)
		*h = glyph.size.y == 0 ?  inst->maxGlyphSize.y : glyph.size.y;
}

void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_font_t* font, ttf_inst_t* inst, uint16_t margin, uint32_t color) {
	if(g == NULL)
		return;
	
	uint16_t out[128];
	int n = utf82unicode((uint8_t*)str, out, 128);

	int32_t ox = x;
	for(int i=0;i <n; i++) {
		if(out[i] == '\n') {
			x = ox;
			y += inst->maxGlyphSize.y;
		}
		else {
			TTY_U16 w = 0;
			graph_draw_char_ttf(g, x, y, out[i], font, inst, color, &w, NULL);
			x += w + margin;
		}
	}
}

#ifdef __cplusplus
}
#endif