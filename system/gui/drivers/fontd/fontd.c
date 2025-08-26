#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include <freetype/freetype.h>
#include "ewoksys/mstr.h"

#define NAME_LEN 64
#define FNAME_LEN 128

typedef struct {
	char name[NAME_LEN];
	char fname[FNAME_LEN];
	FT_Face face;
	//uint32_t current_size;
} ttf_item_t;

#define TTF_MAX  128

static ttf_item_t _ttfs[TTF_MAX];

static FT_Library _library;

#define DO_FONT_CACHE 1
static map_t _font_cache = NULL;

static void font_dev_init(void) {
	_font_cache = hashmap_new();

	FT_Init_FreeType(&_library);
	for(int i=0;i < TTF_MAX; i++)
		memset(&_ttfs[i], 0, sizeof(ttf_item_t));
}

static void font_dev_quit(void) {
	for(int i=0;i < TTF_MAX; i++) {
		if(_ttfs[i].face != NULL)
			FT_Done_Face(_ttfs[i].face);
	}

	if(_font_cache)
		hashmap_free(_font_cache);
}

static char* font_cmd(int from_pid, int argc, char** argv, void* p) {
	if(strcmp(argv[0], "list") == 0) {
		str_t* str = str_new("");
        for(int i=0; i<TTF_MAX; i++) {
            if(_ttfs[i].face != NULL) {
                str_add(str, _ttfs[i].name);
                str_add(str, ", ");
                str_add(str, _ttfs[i].fname);
                str_add(str, "\n");
            }
        }
		char* ret = str_detach(str);
		return ret;
	}
	return NULL;
}

int font_open(const char* name, const char* fname) {
	for(int i=0; i<TTF_MAX; i++) {
		if(_ttfs[i].face != NULL &&
				strcmp(name, _ttfs[i].name) == 0) {
			return i;
		}
	}

	int i;
	for(i=0;i < TTF_MAX; i++) {
		if(_ttfs[i].face == NULL)
			break;
	}
	if(i >= TTF_MAX) {
		return -1;
	}

	if(FT_New_Face(_library, fname, 0, &_ttfs[i].face) != 0)
		return -1;
	strncpy(_ttfs[i].name, name, NAME_LEN-1);
	strncpy(_ttfs[i].fname, fname, FNAME_LEN-1);
	return i;
}

static int font_dev_load(proto_t* in, proto_t* ret) {
	const char* name = proto_read_str(in);
	const char* fname = proto_read_str(in);

	if(name == NULL || fname == NULL) {
		PF->init(ret)->addi(ret, -1);
		return 0;
	}

	int res = font_open(name, fname);
	PF->init(ret)->addi(ret, res);
	return 0;
}

static int free_cache(map_t map, const char* key, any_t data, any_t arg) {
	FT_GlyphSlot slot = (FT_GlyphSlot)data;
	if(slot == NULL)
		return MAP_OK;

	hashmap_remove(map, key);
	if(slot->bitmap.buffer != NULL)
		free(slot->bitmap.buffer);
	free(slot);
	return MAP_OK;
}

static void font_clear_cache(void) {
	if(_font_cache != NULL) {
		hashmap_iterate(_font_cache, free_cache, NULL);	
	}
}

static inline const char* hash_key(int32_t findex, uint32_t c, uint32_t size) {
	static char key[16] = {0};
	snprintf(key, 15, "%d:%d:%x", findex, size, c);
	return key;
}

static int font_fetch_cache(int32_t findex, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
	if(_font_cache == NULL) {
		return -1;
	}

	FT_GlyphSlot p;
	if(hashmap_get(_font_cache, hash_key(findex, c, size), (void**)&p) != 0)
		return -1;
	if(p == NULL)
		return -1;
	memcpy(slot, p, sizeof(FT_GlyphSlotRec));
	return 0;
}

#define MAX_FONT_CACHE 4096

static void font_cache(int32_t findex, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
#if DO_FONT_CACHE
	if(_font_cache == NULL || slot == NULL) {
		return;
	}

	FT_GlyphSlot p = (FT_GlyphSlot)malloc(sizeof(FT_GlyphSlotRec));
	if(p == NULL)
		return;

	if(hashmap_length(_font_cache) >= MAX_FONT_CACHE) {
		font_clear_cache();
	}
	memcpy(p, slot, sizeof(FT_GlyphSlotRec));
	uint32_t sz = slot->bitmap.width * slot->bitmap.rows;
	p->bitmap.buffer = malloc(sz);
	memcpy(p->bitmap.buffer, slot->bitmap.buffer, sz);
	hashmap_put(_font_cache, hash_key(findex, c, size), p);
#endif
}

static FT_Int _load_mode = FT_LOAD_TARGET_LIGHT;
static FT_Int _hinting = 0;
static int font_dev_get_glyph(proto_t* in, proto_t* ret) {
	int findex = proto_read_int(in);
	uint32_t size = (uint32_t)proto_read_int(in);
	if(findex >= TTF_MAX || size == 0 || _ttfs[findex].face == NULL) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	FT_Face face = _ttfs[findex].face; 
	//if(_ttfs[findex].current_size != size) {
		if(FT_Set_Pixel_Sizes(face, 0, size) != 0) {
			PF->init(ret)->addi(ret, -1);
			return -1;
		}
		//_ttfs[findex].current_size = size;
	//}

	uint32_t c = (uint32_t)proto_read_int(in);
	FT_GlyphSlotRec slot;

	if(font_fetch_cache(findex, size, c, &slot) != 0) {
		FT_UInt glyph_index = FT_Get_Char_Index(face, c);
		FT_Int flags = FT_LOAD_RENDER | _hinting | _load_mode;

		if(FT_Load_Glyph(face, glyph_index, flags) != 0) {
			PF->init(ret)->addi(ret, -1);
			return -1;
		}
		memcpy(&slot, face->glyph, sizeof(FT_GlyphSlotRec));
		font_cache(findex, size, c, &slot);
	}

	face_info_t faceinfo;
	faceinfo.ascender = face->size->metrics.ascender;
	faceinfo.descender = face->size->metrics.descender;
	faceinfo.height = face->size->metrics.ascender - face->size->metrics.descender;
	faceinfo.width = face->size->metrics.max_advance;
	uint32_t bmp_size = slot.bitmap.width * slot.bitmap.rows;
	if(bmp_size > 0) {
		PF->format(ret, "m,m,m",
				&slot, sizeof(FT_GlyphSlotRec),
				&faceinfo, sizeof(face_info_t),
				slot.bitmap.buffer, bmp_size);
	}
	else {
		PF->init(ret)->
				add(ret, &slot, sizeof(FT_GlyphSlotRec))->
				add(ret, &faceinfo, sizeof(face_info_t));
	}
	return 0;
}


static int font_dev_get_face(proto_t* in, proto_t* ret) {
	int i = proto_read_int(in);
	uint32_t size = (uint32_t)proto_read_int(in);
	if(i >= TTF_MAX || size == 0 || _ttfs[i].face == NULL) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	FT_Face face = _ttfs[i].face; 
    if(FT_Set_Pixel_Sizes(face, size, 0) != 0) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	face_info_t faceinfo;
	faceinfo.ascender = face->size->metrics.ascender;
	faceinfo.descender = face->size->metrics.descender;
	faceinfo.height = face->size->metrics.ascender - face->size->metrics.descender;
	faceinfo.width = face->size->metrics.max_advance;
	PF->format(ret, "m", &faceinfo, sizeof(face_info_t));
	return 0;
}

static int font_dev_list(proto_t* in, proto_t* ret) {
	for(int i=0; i<TTF_MAX; i++) {
		if(_ttfs[i].face != NULL) {
			PF->adds(ret, _ttfs[i].name);
		}
	}
	return 0;
}

static int font_dev_cntl(int from_pid, int cmd, proto_t* in, proto_t* ret, void* p) {
	(void)from_pid;
	(void)p;

	if(cmd == FONT_DEV_LOAD) {
		return font_dev_load(in, ret);
	}
	else if(cmd == FONT_DEV_GET_GLYPH) {
		return font_dev_get_glyph(in, ret);
	}
	else if(cmd == FONT_DEV_GET_FACE) {
		return font_dev_get_face(in, ret);
	}
	else if(cmd == FONT_DEV_LIST) {
		return font_dev_list(in, ret);
	}
	return -1;
}

static int doargs(int argc, char* argv[]) {
	int c = 0;
	while (c != -1) {
		c = getopt (argc, argv, "lmncvho");
		if(c == -1)
			break;

		switch (c) {
		case 'l':
			_load_mode = FT_LOAD_TARGET_LIGHT;
			break;
		case 'n':
			_load_mode = FT_LOAD_TARGET_NORMAL;
			break;
		case 'h':
			_hinting = FT_LOAD_FORCE_AUTOHINT;
			break;
		case 'o':
			_hinting = FT_LOAD_NO_AUTOHINT;
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_load_mode = FT_LOAD_TARGET_NORMAL;
	_hinting = 0;
	doargs(argc, argv);

	const char* mnt_point = "/dev/font";
	font_dev_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "font");
	dev.dev_cntl = font_dev_cntl;
	dev.cmd = font_cmd;

	font_open(DEFAULT_SYSTEM_FONT, DEFAULT_SYSTEM_FONT_FILE);

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	font_dev_quit();
	return 0;
}
