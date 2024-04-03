#include <ewoksys/vdevice.h>
#include <ewoksys/utf8unicode.h>
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

static int font_load_inst(const char* name, uint16_t ppm, font_inst_t* inst) {
	if(_font_dev_pid < 0)
		return -1;

	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "s,i", name, ppm);

	int ret = -1;
	inst->id = -1;
	inst->cache = NULL;
	if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_LOAD, &in, &out) == 0) {
		inst->id = proto_read_int(&out);
		inst->ppm = ppm;
		inst->max_size.x = proto_read_int(&out);
		inst->max_size.y = proto_read_int(&out);
		inst->cache = hashmap_new();
		ret = 0;
	}

	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

static font_inst_t* get_free_inst(font_t* font) {
	for(int i=0; i<FONT_INST_MAX; i++) {
		if(font->instances[i].id == 0)
			return &font->instances[i];
	}
	return NULL;
}

font_inst_t* font_get_inst(font_t* font, uint16_t size) {
	font_inst_t* inst = NULL;
	for(int i=0; i<FONT_INST_MAX; i++) {
		inst = &font->instances[i];
		if(inst->cache != NULL && inst->ppm == size)
			return inst;
	}

	inst = get_free_inst(font);
	if(inst == NULL)
		return NULL;
	
	if(font_load_inst(font->name, size, inst) != 0)
		return NULL;
	return inst;
}

static int font_load(font_t* font, bool safe) {
	if(_font_dev_pid < 0)
		return -1;

	font_inst_t* inst = get_free_inst(font);
	if(inst == NULL)
		return -1;
	
	if(font_load_inst(font->name, DEFAULT_SYSTEM_FONT_SIZE, inst) == 0)
		return 0;
	if(safe)
		return font_load_inst(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_SIZE, inst);
	return -1;
}

static int free_cache(map_t map, const char* key, any_t data, any_t arg) {
	TTY_Glyph* v = (TTY_Glyph*)data;
	if(v == NULL)
		return MAP_OK;

	hashmap_remove(map, key);
	if(v->cache != NULL)
		free(v->cache);
	free(v);
	return MAP_OK;
}

font_t* font_new(const char* name, bool safe) {
	font_t* font = (font_t*)calloc(sizeof(font_t), 1);
	strncpy(font->name, name, FONT_NAME_MAX-1);
	if(font_load(font, safe) == 0)
		return font;
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

static void font_clear_cache(font_inst_t* inst) {
	if(inst->cache != NULL) {
		hashmap_iterate(inst->cache, free_cache, NULL);	
	}
}

static inline const char* hash_key(uint16_t c) {
	static char key[8] = {0};
	snprintf(key, 7, "%x", c);
	return key;
}

static int font_fetch_cache(font_inst_t* inst, uint16_t c, TTY_Glyph* glyph) {
	if(inst == NULL || inst->cache == NULL) {
		return -1;
	}

	TTY_Glyph* p;
	if(hashmap_get(inst->cache, hash_key(c), (void**)&p) == 0) {
		memcpy(glyph, p, sizeof(TTY_Glyph));
		return 0;
	}	
	return -1;
}

static void font_cache(font_inst_t* inst, uint16_t c, TTY_Glyph* glyph) {
	if(inst == NULL || inst->cache == NULL) {
		return;
	}

	TTY_Glyph* p = (TTY_Glyph*)malloc(sizeof(TTY_Glyph));
	if(p == NULL)
		return;

	if(hashmap_length(inst->cache) >= 4096) {
		font_clear_cache(inst);
	}

	memcpy(p, glyph, sizeof(TTY_Glyph));
	hashmap_put(inst->cache, hash_key(c), p);
}

int font_close(font_t* font) {
	if(_font_dev_pid < 0 || font == NULL)
		return -1;
	
	for(int i=0; i<FONT_INST_MAX; i++) {
		TTY_Glyph* p;
		font_inst_t* inst = &font->instances[i];
		if(inst->cache != NULL) {
			proto_t in;
			PF->init(&in)->addi(&in, inst->id);
			dev_cntl_by_pid(_font_dev_pid, FONT_DEV_CLOSE, &in, NULL);
			PF->clear(&in);

			font_clear_cache(inst);
			hashmap_free(inst->cache);
			inst->cache = NULL;
			inst->id = 0;
		}	
	}
	return 0;
}

TTY_Glyph* font_init_glyph(TTY_Glyph* glyph) {
	memset(glyph, 0, sizeof(glyph));
	return glyph;
}

static int font_get_glyph(font_inst_t* inst, uint16_t c, TTY_Glyph* glyph) {
	if(_font_dev_pid < 0 || inst == NULL)
		return -1;
	font_init_glyph(glyph);

	int ret = -1;
	int do_cache = 0;
	if(font_fetch_cache(inst, c, glyph) != 0) {
		proto_t in, out;
		PF->init(&out);
		PF->format(&in, "i,i", inst->id, c);

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
		font_cache(inst, c, glyph);
	}

	return ret;
}

void font_char_size(uint16_t c, font_t* font, uint16_t size, uint16_t *w, uint16_t* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;
	
	font_inst_t* inst = font_get_inst(font, size);
	if(inst == NULL)
		return;

	TTY_Glyph glyph;
	if(font_get_glyph(inst, c, &glyph) != 0)
		return;
	if(w != NULL)  {
		TTY_S32 sw = glyph.offset.x + 
				(glyph.size.x == 0 ? glyph.advance.x : glyph.size.x);
		if(sw < 0)
			sw = 0;
		*w = sw;
	}

	if(h != NULL) {
		TTY_S32 sh = glyph.offset.y + 
				(glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
		if(sh < 0)
			sh = 0;
		*h = sh;
	}
}

void font_text_size(const char* str,
		font_t* font, uint16_t size, uint32_t *w, uint32_t* h) {
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
		font_char_size(unicode[i], font, size, &cw, NULL);
		int dx = cw;
		if(dx <= 0)
			dx = cw/2;
		x += dx;
	}
	if(w != NULL)
		*w = x;
	if(h != NULL)
		*h = size;
	free(unicode);
}

void graph_draw_char_font(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint16_t size, uint32_t color, TTY_U16* w, TTY_U16* h) {
	if(w != NULL)
		*w = 0;
	if(h != NULL)
		*h = 0;

	font_inst_t* inst = font_get_inst(font, size);
	if(inst == NULL)
		return;

	TTY_Glyph glyph;
	if(font_get_glyph(inst, c, &glyph) != 0)
		return;
	
	if(c == '\t') {
		if(w != NULL)
			*w = glyph.size.x*2;
		if(h != NULL)
			*h = glyph.size.y;
		return;
	}

	if(glyph.cache != NULL) {
		for (TTY_S32 j = 0; j < inst->max_size.y; j++) {
			for (TTY_S32 i = 0; i < inst->max_size.x; i++) {
				TTY_U8 pv = glyph.cache[j*inst->max_size.x+i];
				graph_pixel_argb_safe(g, x+i, y+j,
						(color >> 24) & pv & 0xff,
						(color >> 16) & 0xff,
						(color >> 8) & 0xff,
						color & 0xff);
			}
		}
	}
	if(w != NULL) {
		TTY_S32 sw = glyph.offset.x +
				(glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
		if(sw < 0)
			sw = 0;
		*w = sw;
	}
	if(h != NULL) {
		TTY_S32 sh = glyph.offset.y +
				(glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
		if(sh < 0)
			sh = 0;
		*h = sh;
	}
}

void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint16_t size, uint32_t color, TTY_U16 w, TTY_U16 h) {
	font_inst_t* inst = font_get_inst(font, size);
	if(inst == NULL)
		return;

	TTY_Glyph glyph;
	if(font_get_glyph(inst, c, &glyph) != 0)
		return;

	TTY_U16 fw, fh;
	fw = glyph.offset.x + (glyph.size.x == 0 ?  glyph.advance.x : glyph.size.x);
	fh = glyph.offset.y + (glyph.size.y == 0 ?  glyph.advance.y : glyph.size.y);
	if(w > 0)
		x = x + (((TTY_S32)w) - fw) / 2;
	if(h > 0)
		y = y + (((TTY_S32)h) - fh) / 2;
	
	if(glyph.cache != NULL) {
		for (TTY_S32 j = 0; j < inst->max_size.y; j++) {
			for (TTY_S32 i = 0; i < inst->max_size.x; i++) {
				TTY_U8 pv = glyph.cache[j*inst->max_size.x+i];
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
		font_t* font, uint16_t size, uint32_t color) {
	if(g == NULL || str[0] == 0)
		return;
	
	int len = strlen(str);
	uint16_t* out = (uint16_t*)malloc((len+1)*2);
	int n = utf82unicode((uint8_t*)str, len, out);
	for(int i=0;i <n; i++) {
		TTY_U16 w = 0;
		graph_draw_char_font(g, x, y, out[i], font, size, color, &w, NULL);
		int dx = w;
		if(dx < 0)
			dx = w/2;
		x += dx;
	}
	free(out);
}

void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
		const char* str, font_t* font, uint16_t size, uint32_t color, uint32_t align) {
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