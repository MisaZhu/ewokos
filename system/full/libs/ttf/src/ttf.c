#include <ttf/ttf.h>
#include <sys/utf8unicode.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

ttf_font_t*  ttf_font_load(const char* fname, uint16_t ppm, uint16_t margin) {
	ttf_font_t* font = (ttf_font_t*)malloc(sizeof(ttf_font_t));
   	if(tty_font_init(&font->font, fname)) {
		free(font);
		return NULL;
	}
   	if(tty_instance_init(&font->font, &font->inst, ppm, TTY_INSTANCE_DEFAULT)) {
		free(font);
		return NULL;
	}
	font->margin = margin;
	font->cache = hashmap_new();
	return font;
}

static void ttf_cache(ttf_font_t* font, uint16_t c, TTY_Glyph* glyph) {
	TTY_Glyph* p = (TTY_Glyph*)malloc(sizeof(TTY_Glyph));
	if(p == NULL)
		return;
	memcpy(p, glyph, sizeof(TTY_Glyph));
	hashmap_put(font->cache, (const char*)&c, p);
}

static int free_cache(const char* key, any_t data, any_t arg) {
	map_t* map = (map_t*)arg;
	TTY_Glyph* v = (TTY_Glyph*)data;
	hashmap_remove(map, key);
	free(v);
	return MAP_OK;
}

static void ttf_clear_cache(ttf_font_t* font) {
	hashmap_iterate(font->cache, free_cache, font->cache);	
}

static int ttf_fetch_cache(ttf_font_t* font, uint16_t c, TTY_Glyph* glyph) {
	TTY_Glyph* p;
	if(hashmap_get(font->cache, (const char*)&c, (void**)&p) == 0) {
		memcpy(glyph, p, sizeof(TTY_Glyph));
		return 0;
	}
	return -1;
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
	ttf_clear_cache(font);
	hashmap_free(font->cache);
	tty_instance_free(&font->inst);
	tty_font_free(&font->font);
	free(font);
}

inline int  ttf_font_hight(ttf_font_t* font) {
	return font->inst.maxGlyphSize.y;
}

inline int  ttf_font_width(ttf_font_t* font) {
	return font->inst.maxGlyphSize.x;
}

TTY_Error tty_render_glyph_to_existing_graph(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph, graph_t* g, TTY_U32 x, TTY_U32 y, uint32_t color);
TTY_Error tty_glyph_fetch(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph);

void ttf_char_size(uint16_t c, ttf_font_t* font, uint16_t *w, uint16_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	int do_cache = 0;
	TTY_Glyph glyph;
	if(ttf_fetch_cache(font, c, &glyph) != 0) {
		TTY_U32 glyphIdx;
		if (tty_get_glyph_index(&font->font, c, &glyphIdx))
			return;
		if (tty_glyph_init(&font->font, &glyph, glyphIdx))
			return;
		if(tty_glyph_fetch(&font->font, &font->inst, &glyph))
			return;
		do_cache = 1;
	}
	if(w != NULL)
		*w = glyph.size.x == 0 ?  font->inst.maxGlyphSize.y/2 : glyph.size.x;

	if(h != NULL)
		*h = glyph.size.y == 0 ?  font->inst.maxGlyphSize.y : glyph.size.y;

	if(do_cache)
		ttf_cache(font, c, &glyph);
}

void ttf_text_size(const char* str,
		ttf_font_t* font, uint32_t *w, uint32_t* h) {
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
		x += cw + font->margin;
	}
	if(w != NULL)
		*w = x;
	if(h != NULL)
		*h = font->inst.maxGlyphSize.y;
	free(unicode);
}

void graph_draw_char_ttf_align(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_font_t* font, uint32_t color, TTY_U16 aw, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	int do_cache = 0;
	if(ttf_fetch_cache(font, c, &glyph) != 0) {
		TTY_U32 glyphIdx;
		if (tty_get_glyph_index(&font->font, c, &glyphIdx))
			return;
		if (tty_glyph_init(&font->font, &glyph, glyphIdx))
			return;
		do_cache = 1;
	}

	if(aw != 0) {
		x += (aw-glyph.size.x)/2;
	}

	if(tty_render_glyph_to_existing_graph(&font->font, &font->inst, &glyph, g, x, y, color))
		return;
	if(w != NULL) {
		if(aw > 0)
			*w = aw;
		else
			*w = glyph.size.x == 0 ?  font->inst.maxGlyphSize.y/2 : glyph.size.x;
	}

	if(h != NULL)
		*h = glyph.size.y == 0 ?  font->inst.maxGlyphSize.y : glyph.size.y;
	if(do_cache)
		ttf_cache(font, c, &glyph);
}

inline void graph_draw_char_ttf(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h) {
	graph_draw_char_ttf_align(g, x, y, c,
		font, color, 0, w, h);
}

void graph_draw_text_ttf(graph_t* g, int32_t x, int32_t y, const char* str,
		ttf_font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	
	uint16_t out[128];
	int n = utf82unicode((uint8_t*)str, out, 128);

	for(int i=0;i <n; i++) {
		TTY_U16 w = 0;
		graph_draw_char_ttf(g, x, y, out[i], font, color, &w, NULL);
		x += w + font->margin;
	}
}

#ifdef __cplusplus
}
#endif