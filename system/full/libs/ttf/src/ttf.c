#include <ttf/ttf.h>
#include <sys/utf8unicode.h>
#include <stdlib.h>
#include <string.h>
#include <graph/graph.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

ttf_font_t*  ttf_font_load(const char* fname, uint16_t ppm, int16_t margin) {
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
	if(v->cache != NULL)
		free(v->cache);
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

TTY_Error tty_render_glyph_cache(TTY_Font* font, TTY_Instance* instance, TTY_Glyph* glyph);

static int ttf_render_glyph_cache(TTY_U32 c, ttf_font_t* font, TTY_Glyph* glyph) {
	int do_cache = 0;
	if(ttf_fetch_cache(font, c, glyph) != 0) {
		TTY_U32 glyphIdx;
		if (tty_get_glyph_index(&font->font, c, &glyphIdx))
			return -1;
		if (tty_glyph_init(&font->font, glyph, glyphIdx))
			return -1;
		do_cache = 1;
	}
	if(glyph->cache == NULL) {
		if(tty_render_glyph_cache(&font->font, &font->inst, glyph))
			return -1;
	}
	if(do_cache)
		ttf_cache(font, c, glyph);
	return 0;
}

void ttf_char_size(uint16_t c, ttf_font_t* font, uint16_t *w, uint16_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(c, font, &glyph) != 0)
		return;
	if(w != NULL) 
		*w = glyph.offset.x + (glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
	if(h != NULL)
		*h = glyph.offset.y + (glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
}

void ttf_text_size(const char* str,
		ttf_font_t* font, uint32_t *w, uint32_t* h) {
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
		ttf_char_size(unicode[i], font, &cw, NULL);
		int dx = cw + font->margin;
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
		ttf_font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(c, font, &glyph) != 0)
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
	if(w != NULL)
		*w = glyph.offset.x + (glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
	if(h != NULL)
		*h = glyph.offset.y + (glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
}

void graph_draw_char_ttf_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		ttf_font_t* font, uint32_t color, TTY_U16 w, TTY_U16 h) {
	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(c, font, &glyph) != 0)
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
		ttf_font_t* font, uint32_t color) {
	if(g == NULL || str[0] == 0)
		return;
	
	int len = strlen(str);
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		TTY_U16 w = 0;
		graph_draw_char_ttf(g, x, y, out[i], font, color, &w, NULL);
		int dx = w + font->margin;
		if(dx <= 0)
			dx = w/2;
		x += dx;
	}
	free(out);
}

#ifdef __cplusplus
}
#endif