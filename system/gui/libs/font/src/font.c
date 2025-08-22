#include <ewoksys/vdevice.h>
#include <ewoksys/utf8unicode.h>
#include <ewoksys/mstr.h>
#include <font/font.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FACE_PIXEL_DENT   64

static int free_cache(map_t map, const char* key, any_t data, any_t arg) {
	FT_GlyphSlot slot = (FT_GlyphSlot)data;
	if(slot == NULL)
		return MAP_OK;

	hashmap_remove(map, key);
	if(slot->bitmap.buffer != NULL)
		free(slot->bitmap.buffer);
	if(slot->other != NULL)
		free(slot->other);
	free(slot);
	return MAP_OK;
}

font_t* font_new(const char* name, bool safe) {
	if(font_init() != 0) 
		return NULL;

	font_t* font = (font_t*)calloc(sizeof(font_t), 1);
	strncpy(font->name, name, FONT_NAME_MAX-1);
	int id = font_load(name, "");
	if(id >= 0) {
		font->cache = hashmap_new();
		font->id = id;
		return font;
	}
	if(safe) {
		id = font_load(DEFAULT_SYSTEM_FONT, "");
		if(id >= 0) {
			strncpy(font->name, DEFAULT_SYSTEM_FONT, FONT_NAME_MAX-1);
			font->id = id;
			font->cache = hashmap_new();
			return font;
		}
	}
	free(font);
	return NULL;
}

int font_free(font_t* font) {
	if(font == NULL)
		return -1;
	font_close(font);
	free(font);
	return 0;
}

static void font_clear_cache(font_t* font) {
	if(font->cache != NULL) {
		hashmap_iterate(font->cache, free_cache, NULL);	
	}
}

int font_close(font_t* font) {
	if(font == NULL)
		return -1;

	font_clear_cache(font);
	hashmap_free(font->cache);
	font->cache = NULL;
	return 0;
}

static inline const char* hash_key(uint32_t c, uint32_t size) {
	static char key[16] = {0};
	snprintf(key, 15, "%d:%x", size, c);
	return key;
}

static int font_fetch_cache(font_t* font, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
	if(font == NULL || font->cache == NULL) {
		return -1;
	}

	FT_GlyphSlot p;
	if(hashmap_get(font->cache, hash_key(c, size), (void**)&p) == 0) {
		if(p != NULL)
			memcpy(slot, p, sizeof(FT_GlyphSlotRec));
		return 0;
	}	
	return -1;
}

#define MAX_FONT_CACHE 4096

static void font_cache(font_t* font, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
	if(font == NULL || font->cache == NULL) {
		return;
	}

	FT_GlyphSlot p = NULL;
	if(slot != NULL) {
		p = (FT_GlyphSlot)malloc(sizeof(FT_GlyphSlotRec));
		if(p == NULL)
			return;

		if(hashmap_length(font->cache) >= MAX_FONT_CACHE) {
			font_clear_cache(font);
		}
		memcpy(p, slot, sizeof(FT_GlyphSlotRec));
	}

	hashmap_put(font->cache, hash_key(c, size), p);
}

FT_GlyphSlot font_init_glyph(FT_GlyphSlot glyph) {
	memset(glyph, 0, sizeof(FT_GlyphSlotRec));
	return glyph;
}

int  font_get_height(font_t* font, uint32_t size) {
	face_info_t face;
	if(font_get_face(font, size, &face) != 0)
		return 0;
	return face.height/FACE_PIXEL_DENT;
}

int  font_get_width(font_t* font, uint32_t size) {
	face_info_t face;
	if(font_get_face(font, size, &face) != 0)
		return 0;
	return face.width/FACE_PIXEL_DENT;
}

static int font_get_glyph(font_t* font, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
	if(font == NULL)
		return -1;
	memset(slot, 0, sizeof(FT_GlyphSlotRec));

	int ret = -1;
	int do_cache = 0;
	if(font_fetch_cache(font, size, c, slot) != 0) {
		if(font_get_glyph_info(font, size, c, slot) == 0) {
			ret = 0;
			do_cache = 1;
		}
	}
	else {
		ret = 0;
	}

	if(do_cache) {
		font_cache(font, size, c, slot);
	}

	return ret;
}

void font_char_size(uint32_t c, font_t* font, uint32_t size, uint32_t *w, uint32_t* h) {
	if(w != NULL)
		*w = size/2;
	if(h != NULL)
		*h = size;

	FT_GlyphSlotRec slot;
	if(font_get_glyph(font, size, c, &slot) != 0)
		return;
	face_info_t* faceinfo = (face_info_t*)slot.other;
	if(w != NULL)  {
		*w = slot.bitmap_left + slot.bitmap.width;
		if((*w) == 0)
			*w = size/2;
		/*
		*w = faceinfo->width/FACE_PIXEL_DENT;
		if(*w > size)
			*w = size;
			*/
	}

	if(h != NULL && faceinfo != NULL) {
		*h = (faceinfo->height/FACE_PIXEL_DENT);
		if((*h) == 0)
			*h = size;
	}
}

void font_text_size(const char* str,
		font_t* font, uint32_t size, uint32_t *w, uint32_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;
	
	int sz = strlen(str);
	uint32_t* unicode = (uint32_t*)malloc((sz+1)*2);
	if(unicode == NULL)
		return;

	int n = utf82unicode((uint8_t*)str, sz, (unsigned short *)unicode);

	int32_t x = 0;
	uint32_t th = 0;
	
	for(int i=0;i <n; i++) {
		uint32_t cw = 0;
		uint32_t ch = 0;
		font_char_size(unicode[i], font, size, &cw, &ch);
		x += cw;
		if(th < ch)
			th = ch;
	}
	if(w != NULL)
		*w = x;
	if(h != NULL)
		*h = th;
	free(unicode);
}

void graph_draw_unicode_font(graph_t* g, int32_t x, int32_t y, uint32_t c,
		font_t* font, uint32_t size, uint32_t color, uint32_t *w, uint32_t *h) {
	if(w != NULL)
		*w = size/2;
	if(h != NULL)
		*h = size;

	FT_GlyphSlotRec slot;
	if(font_get_glyph(font, size, c, &slot) != 0)
		return;
	face_info_t* faceinfo = (face_info_t*)slot.other;

	x += slot.bitmap_left;
	y = y - slot.bitmap_top + (faceinfo->ascender/FACE_PIXEL_DENT);
	
	if(c == '\t') {
		if(w != NULL) {
			*w = slot.bitmap.width*2;
			if((*w) == 0)
				*w = size;
		}
		if(h != NULL) {
			*h = (faceinfo->height/FACE_PIXEL_DENT);
			if((*h) == 0)
				*h = size;
		}
		return;
	}

	if(slot.bitmap.buffer != NULL) {
		for (int32_t j = 0; j < slot.bitmap.rows; j++) {
			for (int32_t i = 0; i < slot.bitmap.width; i++) {
				uint8_t pv = slot.bitmap.buffer[j*slot.bitmap.width+i];
				graph_pixel_argb_safe(g, x+i, y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}

	if(h != NULL) {
		*h = (faceinfo->height/FACE_PIXEL_DENT);
		if((*h) == 0)
			*h = size;
	}

	if(w != NULL) {
		*w = slot.bitmap_left + slot.bitmap.width;
		if((*w) == 0)
			*w = size/2;
		/*
		*w = faceinfo->width/FACE_PIXEL_DENT;
		if(*w > size)
			*w = size;
			*/
	}
}

void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, uint32_t c,
		font_t* font, uint32_t size, uint32_t color, uint32_t w, uint32_t h) {

	FT_GlyphSlotRec slot;
	if(font_get_glyph(font, size, c, &slot) != 0)
		return;
	face_info_t* faceinfo = (face_info_t*)slot.other;

	x += ((int)w - ((int)slot.bitmap.width - slot.bitmap_left))/2;
	y = y - slot.bitmap_top + (faceinfo->ascender/FACE_PIXEL_DENT);

	if(slot.bitmap.buffer != NULL) {
		for (int32_t j = 0; j < slot.bitmap.rows; j++) {
			for (int32_t i = 0; i < slot.bitmap.width; i++) {
				uint8_t pv = slot.bitmap.buffer[j*slot.bitmap.width+i];
				graph_pixel_argb_safe(g, 
						x+i,
						y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}
}

void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
		font_t* font, uint32_t size, uint32_t color) {
	if(g == NULL || str[0] == 0)
		return;
	
	int len = strlen(str);
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		uint32_t w = 0;
		graph_draw_unicode_font(g, x, y, out[i], font, size, color, &w, NULL);
		int dx = w;
		if(dx < 0)
			dx = w/2;
		x += dx;
	}
	free(out);
}

void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
		const char* str, font_t* font, uint32_t size, uint32_t color, uint32_t align) {
	int32_t fw, fh;
	font_text_size(str, font, size, (uint32_t*)&fw, (uint32_t*)&fh);
	if((align & FONT_ALIGN_CENTER) != 0) {
		x = x + (w-fw)/2;
		y = y + (h-fh)/2;
	}
	graph_draw_text_font(g, x, y, str, font, size, color);
}

#ifdef __cplusplus
}
#endif