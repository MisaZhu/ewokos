#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <ewoksys/vfs.h>
#include <ttf/ttf.h>
#include <ewoksys/vdevice.h>
#include <ewoksys/syscall.h>
#include <ewoksys/klog.h>
#include <font/font.h>

#define NAME_LEN 128

typedef struct {
	char name[NAME_LEN];
	ttf_t* ttf;
	int32_t ref;
} ttf_item_t;

typedef struct {
	int ttf_index;
	ttf_font_t* font;
} font_item_t;


#define TTF_MAX   8
#define FONT_MAX  32
static ttf_item_t _ttfs[TTF_MAX];
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

static int font_fetch(const char* fname, int ppm, int* ttf_index) {
	*ttf_index  = -1;
	for(int i=0; i<TTF_MAX; i++) {
		if(_ttfs[i].ttf != NULL &&
				strcmp(fname, _ttfs[i].name) == 0) {
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

static int font_open(const char* fname, int ppm, int ttf_index) {
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
		sstrncpy(_ttfs[i].name, fname, NAME_LEN-1);
		ttf_index = i;
	}

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

static int font_dev_load(proto_t* in, proto_t* ret) {
	const char* fname = proto_read_str(in);
	int ppm = proto_read_int(in);

	int i = -1;
	int at = font_fetch(fname, ppm, &i);
	if(at < 0)
		at = font_open(fname, ppm, i);
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
		PF->init(ret)->addi(ret, -1);
		return -1;
	}
	if(glyph.cache != NULL) {
		PF->init(ret)->
			add(ret, &glyph, sizeof(TTY_Glyph))->
			add(ret, glyph.cache,
					font->inst.maxGlyphSize.x*
					font->inst.maxGlyphSize.y);
	}
	else {
		PF->init(ret)->
			add(ret, &glyph, sizeof(TTY_Glyph));
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
	return -1;
}

int main(int argc, char** argv) {
	const char* mnt_point = argc > 1 ? argv[1]: "/dev/font";
	font_dev_init();

	vdevice_t dev;
	memset(&dev, 0, sizeof(vdevice_t));
	strcpy(dev.name, "font");
	dev.dev_cntl = font_dev_cntl;

	for(int i=2; i<argc; i++) {
		klog("    pre-load: %s ... ", argv[i]);
		font_open(argv[i], 12, -1);
		klog("ok\n");
	}

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	font_dev_quit();
	return 0;
}
