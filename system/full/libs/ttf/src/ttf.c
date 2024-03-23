#include <ttf/ttf.h>
#include <ewoksys/utf8unicode.h>
#include <stdlib.h>
#include <string.h>
#include <graph/graph.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

ttf_t* ttf_load(const char* fname) {
	ttf_t* ttf = (ttf_t*)malloc(sizeof(ttf_t));
	memset(ttf, 0, sizeof(ttf_t));
	if(tty_font_init(ttf, fname)) {
		free(ttf);
		return NULL;
	}
	return ttf;
}

ttf_font_t*  ttf_new_inst(ttf_t* ttf, uint16_t ppm) {
	ttf_font_t* font = (ttf_font_t*)malloc(sizeof(ttf_font_t));
	memset(font, 0, sizeof(ttf_font_t));
   	if(tty_instance_init(ttf, &font->inst, ppm, TTY_INSTANCE_DEFAULT)) {
		free(font);
		return NULL;
	}
	return font;
}

int ttf_font_resize(ttf_t* ttf, ttf_font_t* font, uint16_t ppm) {
	if(font == NULL || ttf == NULL)
		return -1;
	if(tty_instance_resize(ttf, &font->inst, ppm))
		return -1;
	return 0;
}

void ttf_free(ttf_t* ttf) {
	tty_font_free(ttf);
	free(ttf);
}

void  ttf_font_free(ttf_font_t* font) {
	if(font == NULL)
		return;
	tty_instance_free(&font->inst);
}

inline int  ttf_font_hight(ttf_font_t* font) {
	return font->inst.maxGlyphSize.y;
}

inline int  ttf_font_width(ttf_font_t* font) {
	return font->inst.maxGlyphSize.x;
}

TTY_Error tty_render_glyph_cache(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph);

int ttf_render_glyph_cache(ttf_t* ttf, ttf_font_t* font, TTY_U32 c, TTY_Glyph* glyph) {
	int do_cache = 0;
	memset(glyph, 0, sizeof(TTY_Glyph));
	TTY_U32 glyphIdx;
	if (tty_get_glyph_index(ttf, c, &glyphIdx))
		return -1;
	if (tty_glyph_init(ttf, glyph, glyphIdx))
		return -1;
	if(tty_render_glyph_cache(ttf, &font->inst, glyph))
		return -1;
	return 0;
}

void ttf_char_size(ttf_t* ttf, ttf_font_t* font, uint16_t c, uint16_t *w, uint16_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(ttf, font, c, &glyph) != 0)
		return;
	if(w != NULL)  {
		*w = glyph.offset.x + (glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
		if(*w < 0)
			*w = 0;
	}

	if(h != NULL) {
		*h = glyph.offset.y + (glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
		if(*h < 0)
			*h = 0;
	}
}

void ttf_text_size(ttf_t* ttf,
		ttf_font_t* font,
		const char* str,
		uint32_t *w, uint32_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;
	
	int sz = strlen(str);
	uint16_t* unicode = (uint16_t*)malloc((sz+1)*2);
	if(unicode == NULL)
		return;

	int n = utf82unicode((uint8_t*)str, sz, unicode);

	int32_t x = 0;
	for(int i=0;i <n; i++) {
		TTY_U16 cw = 0;
		ttf_char_size(ttf, font, unicode[i], &cw, NULL);
		int dx = cw;
		if(dx <= 0)
			dx = cw/2;
		x += dx;
	}
	if(w != NULL)
		*w = x;
	if(h != NULL)
		*h = font->inst.maxGlyphSize.y;
	free(unicode);
}

void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_t* ttf, ttf_font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(ttf, font, c, &glyph) != 0)
		return;
	
	if(glyph.cache != NULL) {
		for (TTY_S32 j = 0; j < font->inst.maxGlyphSize.y; j++) {
			for (TTY_S32 i = 0; i < font->inst.maxGlyphSize.x; i++) {
				TTY_U8 pv = glyph.cache[j*font->inst.maxGlyphSize.x+i];
				graph_pixel_argb_safe(g, x+i, y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}
	if(w != NULL) {
		*w = glyph.offset.x + (glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
		if(*w < 0)
			*w = 0;
	}
	if(h != NULL) {
		*h = glyph.offset.y + (glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
		if(*h < 0)
			*h = 0;
	}
}

void graph_draw_char_ttf_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_t* ttf, ttf_font_t* font, uint32_t color, TTY_U16 w, TTY_U16 h) {
	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(ttf, font, c, &glyph) != 0)
		return;

	if(w > 0)
		x += (((TTY_S32)w) - glyph.size.x)/2 - glyph.offset.x;
	if(h > 0)
		y += (((TTY_S32)h) - glyph.size.y)/2 - glyph.offset.y;
	
	if(glyph.cache != NULL) {
		for (TTY_S32 j = 0; j < font->inst.maxGlyphSize.y; j++) {
			for (TTY_S32 i = 0; i < font->inst.maxGlyphSize.x; i++) {
				TTY_U8 pv = glyph.cache[j*font->inst.maxGlyphSize.x+i];
				graph_pixel_argb_safe(g, x+i, y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}
}

void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_t* ttf, ttf_font_t* font, uint32_t color) {
	if(g == NULL || str[0] == 0)
		return;
	
	int len = strlen(str);
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		TTY_U16 w = 0;
		graph_draw_char_ttf(g, x, y, out[i], ttf, font, color, &w, NULL);
		int dx = w;
		if(dx <= 0)
			dx = w/2;
		x += dx;
	}
	free(out);
}

#ifdef __cplusplus
}
#endif