#include <sys/vdevice.h>
#include <font/font.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static int _font_dev_pid = -1;

int font_init(void) {
	_font_dev_pid = dev_get_pid("/dev/font");
	if(_font_dev_pid < 0)
		return -1;
	return 0;
}

int font_load(const char* fname, uint16_t ppm, font_t* font) {
	if(_font_dev_pid < 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->init(&in)->adds(&in, fname)->addi(&in, ppm);

	int ret = -1;
	font->id = -1;
	font->cache = NULL;
	if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_LOAD, &in, &out) == 0) {
		font->id = proto_read_int(&out);
		font->max_size.x = proto_read_int(&out);
		font->max_size.y = proto_read_int(&out);
		font->cache = hashmap_new();
		ret = 0;
	}

	PF->clear(&in);
	PF->clear(&out);
	return ret;
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

static void font_clear_cache(font_t* font) {
	hashmap_iterate(font->cache, free_cache, font->cache);	
}

static int font_fetch_cache(font_t* font, uint16_t c, TTY_Glyph* glyph) {
	TTY_Glyph* p;
	if(hashmap_get(font->cache, (const char*)&c, (void**)&p) == 0) {
		memcpy(glyph, p, sizeof(TTY_Glyph));
		return 0;
	}
	return -1;
}

static void font_cache(font_t* font, uint16_t c, TTY_Glyph* glyph) {
	TTY_Glyph* p = (TTY_Glyph*)malloc(sizeof(TTY_Glyph));
	if(p == NULL)
		return;
	memcpy(p, glyph, sizeof(TTY_Glyph));
	hashmap_put(font->cache, (const char*)&c, p);
}

int font_close(font_t* font) {
	if(_font_dev_pid < 0 || font == NULL)
		return -1;
	
	if(font->cache != NULL) {
		font_clear_cache(font);
		hashmap_free(font->cache);
	}

	proto_t in;
	PF->init(&in)->addi(&in, font->id);

	dev_cntl_by_pid(_font_dev_pid, FONT_DEV_CLOSE, &in, NULL);
	PF->clear(&in);
	return 0;
}

TTY_Glyph* font_init_glyph(TTY_Glyph* glyph) {
	memset(glyph, 0, sizeof(glyph));
	return glyph;
}

int font_get_glyph(font_t* font, uint16_t c, TTY_Glyph* glyph) {
	if(_font_dev_pid < 0 || font == NULL)
		return -1;
	font_init_glyph(glyph);

	int ret = -1;
	int do_cache = 0;
	if(font_fetch_cache(font, c, glyph) != 0) {
		proto_t in, out;
		PF->init(&out);
		PF->init(&in)->addi(&in, font->id)->addi(&in, c);

		if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_GET, &in, &out) == 0) {
			proto_read_to(&out, glyph, sizeof(TTY_Glyph));
			int32_t sz;
			void* p = proto_read(&out, &sz);
			if(p == NULL || sz <= 0) {
				glyph->cache = NULL;
			}
			else {
				glyph->cache = malloc(sz);
				memcpy(glyph->cache, p, sz);
			}
			ret = 0;
			do_cache = 1;
		}
		PF->clear(&in);
		PF->clear(&out);
	}
	else {
		ret = 0;
	}

	if(do_cache) {
		font_cache(font, c, glyph);
	}

	return ret;
}

void font_char_size(uint16_t c, font_t* font,
		uint16_t *w, uint16_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	if(font_get_glyph(font, c, &glyph) != 0)
		return;
	if(w != NULL)  {
		*w = glyph.offset.x + 
				(glyph.size.x == 0 ? glyph.advance.x : glyph.size.x);
		if(*w < 0)
			*w = 0;
	}

	if(h != NULL) {
		*h = glyph.offset.y + 
				(glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
		if(*h < 0)
			*h = 0;
	}
}

void font_text_size(const char* str,
		font_t* font, uint32_t *w, uint32_t* h) {
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
	uint16_t maxh;
	for(int i=0;i <n; i++) {
		TTY_U16 cw = 0;
		font_char_size(unicode[i], font, &cw, NULL);
		int dx = cw;
		if(dx <= 0)
			dx = cw/2;
		x += dx;
	}
	if(w != NULL)
		*w = x;
	if(h != NULL)
		*h = font->max_size.y;
	free(unicode);
}

void graph_draw_char_font(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	TTY_Glyph glyph;
	if(font_get_glyph(font, c, &glyph) != 0)
		return;
	
	if(c == '\t') {
		if(w != NULL)
			*w = glyph.size.x*2;
		if(h != NULL)
			*h = glyph.size.y;
		return;
	}

	if(glyph.cache != NULL) {
		for (TTY_S32 j = 0; j < font->max_size.y; j++) {
			for (TTY_S32 i = 0; i < font->max_size.x; i++) {
				TTY_U8 pv = glyph.cache[j*font->max_size.x+i];
				graph_pixel_argb_safe(g, x+i, y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}
	if(w != NULL) {
		*w = glyph.offset.x +
				(glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
		if(*w < 0)
			*w = 0;
	}
	if(h != NULL) {
		*h = glyph.offset.y +
				(glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
		if(*h < 0)
			*h = 0;
	}
}

void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint32_t color, TTY_U16 w, TTY_U16 h) {
	TTY_Glyph glyph;
	if(font_get_glyph(font, c, &glyph) != 0)
		return;
	TTY_U16 fw, fh;
	fw = glyph.offset.x + (glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
	fh = glyph.offset.y + (glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
	if(w > 0)
		x = x + (((TTY_S32)w) - fw) / 2;
	if(h > 0)
		y = y + (((TTY_S32)h) - fh) / 2;
	
	if(glyph.cache != NULL) {
		for (TTY_S32 j = 0; j < font->max_size.y; j++) {
			for (TTY_S32 i = 0; i < glyph.size.x; i++) {
				TTY_U8 pv = glyph.cache[j*font->max_size.x+i];
				graph_pixel_argb_safe(g, x+i, y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}
}

void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
		font_t* font, uint32_t color) {
	if(g == NULL || str[0] == 0)
		return;
	
	int len = strlen(str);
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		TTY_U16 w = 0;
		graph_draw_char_font(g, x, y, out[i], font, color, &w, NULL);
		int dx = w;
		if(dx < 0)
			dx = w/2;
		x += dx;
	}
	free(out);
}

void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
		const char* str, font_t* font, uint32_t color, uint32_t align) {
	int32_t fw, fh;
	font_text_size(str, font, &fw, &fh);
	if((align & FONT_ALIGN_CENTER) != 0) {
		x = x + (w-fw)/2;
		y = y + (h-fh)/2;
	}
	graph_draw_text_font(g, x, y, str, font, color);
}

#ifdef __cplusplus
}
#endif