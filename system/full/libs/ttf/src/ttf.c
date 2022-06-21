#include <ttf/ttf.h>
#include <sys/utf8unicode.h>
#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int  ttf_font_load(ttf_font_t* font, const char* fname, uint16_t ppm) {
   	if(tty_font_init(&font->font, fname))
		return -1;
   	if(tty_instance_init(&font->font, &font->inst, ppm, TTY_INSTANCE_DEFAULT))
		return -1;
	return 0;
}

int  ttf_font_resize(ttf_font_t* font, uint16_t ppm) {
	if(font == NULL)
		return -1;
	if(tty_instance_resize(&font->font, &font->inst, ppm))
		return -1;
	return 0;
}

void  ttf_font_free(ttf_font_t* font) {
	if(font == NULL)
		return;
	tty_instance_free(&font->inst);
	tty_font_free(&font->font);
}

int  ttf_font_hight(ttf_font_t* font) {
	return font->inst.maxGlyphSize.y;
}

int  ttf_font_width(ttf_font_t* font) {
	return font->inst.maxGlyphSize.x;
}

TTY_Error tty_render_glyph_to_existing_graph(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph, graph_t* g, TTY_U32 x, TTY_U32 y, uint32_t color);
TTY_Error tty_glyph_fetch(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph);

void ttf_char_size(uint16_t c, ttf_font_t* font, uint16_t *w, uint16_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_U32 glyphIdx;
	if (tty_get_glyph_index(&font->font, c, &glyphIdx))
		return;

	TTY_Glyph glyph;
	if (tty_glyph_init(&font->font, &glyph, glyphIdx))
		return;

	if(tty_glyph_fetch(&font->font, &font->inst, &glyph))
		return;
	if(w != NULL)
		*w = glyph.size.x == 0 ?  font->inst.maxGlyphSize.x : glyph.size.x;

	if(h != NULL)
		*h = glyph.size.y == 0 ?  font->inst.maxGlyphSize.y : glyph.size.y;
}

void ttf_text_size(const char* str,
		ttf_font_t* font, uint16_t margin, uint32_t *w, uint32_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	int sz = strlen(str)+1;
	uint16_t* unicode = (uint16_t*)malloc(sz*2);
	if(unicode == NULL)
		return;

	int n = utf82unicode((uint8_t*)str, unicode, sz);

	int32_t x = 0;
	for(int i=0;i <n; i++) {
		TTY_U16 cw = 0;
		ttf_char_size(unicode[i], font, &cw, NULL);
		x += cw + margin;
	}
	if(w != NULL)
		*w = x;
	if(h != NULL)
		*h = font->inst.maxGlyphSize.y;
	free(unicode);
}

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_U32 glyphIdx;
	if (tty_get_glyph_index(&font->font, c, &glyphIdx))
		return;

	TTY_Glyph glyph;
	if (tty_glyph_init(&font->font, &glyph, glyphIdx))
		return;

	if(tty_render_glyph_to_existing_graph(&font->font, &font->inst, &glyph, g, x, y, color))
		return;
	if(w != NULL)
		*w = glyph.size.x == 0 ?  font->inst.maxGlyphSize.x : glyph.size.x;

	if(h != NULL)
		*h = glyph.size.y == 0 ?  font->inst.maxGlyphSize.y : glyph.size.y;
}

void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_font_t* font, uint16_t margin, uint32_t color) {
	if(g == NULL)
		return;
	
	uint16_t out[128];
	int n = utf82unicode((uint8_t*)str, out, 128);

	for(int i=0;i <n; i++) {
		TTY_U16 w = 0;
		graph_draw_char_ttf(g, x, y, out[i], font, color, &w, NULL);
		x += w + margin;
	}
}

#ifdef __cplusplus
}
#endif