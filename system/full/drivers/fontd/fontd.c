#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <pthread.h>
#include <font/font.h>
#include <tinyjson/tinyjson.h>
#include "fontd.h"

ttf_item_t _ttfs[TTF_MAX];

typedef struct {
	int ttf_index;
	ttf_font_t* font;
} font_item_t;

#define FONT_MAX  32
static font_item_t _fonts[FONT_MAX];

static void font_dev_init(void) {
	for(int i=0;i < TTF_MAX; i++)
		memset(&_ttfs[i], 0, sizeof(ttf_item_t));
	for(int i=0;i < FONT_MAX; i++)
		memset(&_fonts[i], 0, sizeof(font_item_t));
}

static void font_dev_quit(void) {
	for(int i=0;i < FONT_MAX; i++) {
		if(_fonts[i].font == NULL)
		ttf_font_free(_fonts[i].font);
	}

	for(int i=0;i < TTF_MAX; i++) {
		if(_ttfs[i].ttf == NULL)
		ttf_free(_ttfs[i].ttf);
	}
}

static int font_fetch(const char* name, int ppm, int* ttf_index) {
	*ttf_index  = -1;
	for(int i=0; i<TTF_MAX; i++) {
		if(_ttfs[i].ttf != NULL &&
				strcmp(name, _ttfs[i].name) == 0) {
			*ttf_index= i;
			break;
		}
	}

	for(int i=0;i < FONT_MAX; i++) {
		font_item_t* font = &_fonts[i];
		if(font->ttf_index == *ttf_index && font->font->inst.ppem == ppm) 
			return i;
	}
	return -1;
}

static int font_open_size(int ttf_index, int ppm) {
	if(ttf_index < 0) //TTF not loaded.
		return -1;

	int j;
	for(j=0;j < FONT_MAX; j++) {
		font_item_t* font = &_fonts[j];
		if(font->font == NULL)
			break;
	}
	if(j >= FONT_MAX)
		return -1;
	
	ttf_font_t* font = ttf_new_inst(_ttfs[ttf_index].ttf, ppm);
	if(font == NULL)
		return -1;
	_fonts[j].font = font;
	_fonts[j].ttf_index = ttf_index;
	_ttfs[ttf_index].ref++;
	return j;
}

int font_open(const char* name, const char* fname, int ppm, int ttf_index) {
	if(ttf_index < 0) { //TTF not loaded.
		int i;
		for(i=0;i < TTF_MAX; i++) {
			if(_ttfs[i].ttf == NULL)
				break;
		}
		if(i >= TTF_MAX)
			return -1;

		ttf_t* ttf = ttf_load(fname);
		if(ttf == NULL)
			return -1;

		_ttfs[i].ttf = ttf;
		strncpy(_ttfs[i].name, name, NAME_LEN-1);
		strncpy(_ttfs[i].fname, fname, FNAME_LEN-1);
		ttf_index = i;
	}

	return font_open_size(ttf_index, ppm);
}

static int font_dev_load(proto_t* in, proto_t* ret) {
	const char* name = proto_read_str(in);
	int ppm = proto_read_int(in);

	int i = -1;
	int at = font_fetch(name, ppm, &i);
	if(at < 0 && i >= 0)
		at = font_open_size(i, ppm);

	if(at >= 0) {
		PF->init(ret)->addi(ret, at)->
				addi(ret, _fonts[at].font->inst.maxGlyphSize.x)->
				addi(ret, _fonts[at].font->inst.maxGlyphSize.y);
	}
	else {
		PF->init(ret)->addi(ret, -1);
	}
	return 0;
}

static int font_dev_close(proto_t* in, proto_t* ret) {
	int i = proto_read_int(in);
	if(i >= FONT_MAX)
		return -1;

	int j = _fonts[i].ttf_index;
	if(j >=0 && j < TTF_MAX)
		_ttfs[j].ref--;
	return 0;
}

static int font_dev_get(proto_t* in, proto_t* ret) {
	int i = proto_read_int(in);
	ttf_t* ttf;
	ttf_font_t* font;
	if(i >= FONT_MAX || _fonts[i].font == NULL ||
			_fonts[i].ttf_index >= TTF_MAX) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	ttf = _ttfs[_fonts[i].ttf_index].ttf;
	font = _fonts[i].font;

	TTY_U32 c = (TTY_U32)proto_read_int(in);
	TTY_Glyph glyph;
	if(ttf == NULL || font == NULL ||
			ttf_render_glyph_cache(ttf, font, c, &glyph) != 0) {
		if(glyph.cache != NULL)
			free(glyph.cache);
		PF->init(ret)->addi(ret, -1);
		return -1;
	}
	if(glyph.cache != NULL) {
		PF->format(ret, "m,m",
				&glyph, sizeof(TTY_Glyph),
				glyph.cache, font->inst.maxGlyphSize.x*font->inst.maxGlyphSize.y);
		free(glyph.cache);
	}
	else {
		PF->init(ret)->add(ret, &glyph, sizeof(TTY_Glyph));
	}
	return 0;
}

static int font_dev_list(proto_t* in, proto_t* ret) {
	uint32_t i = 0;
	for(int i=0; i<TTF_MAX; i++) {
		if(_ttfs[i].ttf != NULL) {
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
	else if(cmd == FONT_DEV_CLOSE) {
		return font_dev_close(in, ret);
	}
	else if(cmd == FONT_DEV_GET) {
		return font_dev_get(in, ret);
	}
	else if(cmd == FONT_DEV_LIST) {
		return font_dev_list(in, ret);
	}
	return -1;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/font";
	font_dev_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "font");
	dev.dev_cntl = font_dev_cntl;
	dev.cmd = font_cmd;

	load_config();

	device_run(&dev, mnt_point, FS_TYPE_CHAR, 0444);
	font_dev_quit();
	return 0;
}
