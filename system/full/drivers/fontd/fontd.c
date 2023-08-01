#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <ttf/ttf.h>
#include <sys/vdevice.h>
#include <sys/syscall.h>
#include <sys/klog.h>
#include <font/font.h>

#define NAME_LEN 128
typedef struct {
	char name[NAME_LEN];
	int32_t ppm;
	ttf_font_t* font;
	int32_t ref;
} font_item_t;

#define FONT_MAX  16
static font_item_t _fonts[FONT_MAX];

static void font_dev_init(void) {
	for(int i=0;i < FONT_MAX; i++)
		memset(&_fonts[i], 0, sizeof(font_item_t));
}

static void font_dev_quit(void) {
	for(int i=0;i < FONT_MAX; i++) {
		if(_fonts[i].font != NULL)
			ttf_font_free(_fonts[i].font);
	}
}

static int font_fetch(const char* fname, int ppm) {
	for(int i=0;i < FONT_MAX; i++) {
		if(_fonts[i].font != NULL &&
				strcmp(fname, _fonts[i].name) == 0 &&
				_fonts[i].ppm == ppm)
		return i;
	}
	return -1;
}

static int font_open(const char* fname, int ppm) {
	int i;
	for(i=0;i < FONT_MAX; i++) {
		if(_fonts[i].font == NULL)
			break;
	}
	if(i >= FONT_MAX)
		return -1;

	ttf_font_t* font = ttf_font_load(fname, ppm);
	if(font == NULL)
		return -1;

	_fonts[i].font = font;
	strncpy(_fonts[i].name, fname, NAME_LEN-1);
	_fonts[i].ppm = ppm;
	_fonts[i].ref++;
	return i;
}

static int font_dev_load(proto_t* in, proto_t* ret) {
	const char* fname = proto_read_str(in);
	int ppm = proto_read_int(in);

	int i = font_fetch(fname, ppm);
	if(i < 0)
		i = font_open(fname, ppm);
	if(i >= 0) {
		PF->init(ret)->addi(ret, i)->
				addi(ret, _fonts[i].font->inst.maxGlyphSize.x)->
				addi(ret, _fonts[i].font->inst.maxGlyphSize.y);
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

	if(_fonts[i].font != NULL) {
		_fonts[i].ref--;
	}
	/*if(_fonts[i].font != NULL) {
		ttf_font_free(_fonts[i].font);
		memset(&_fonts[i], 0, sizeof(font_item_t));
	}
	*/
	PF->init(ret);
	return 0;
}

static int font_dev_get(proto_t* in, proto_t* ret) {
	int i = proto_read_int(in);
	if(i >= FONT_MAX || _fonts[i].font == NULL) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	TTY_U32 c = (TTY_U32)proto_read_int(in);
	TTY_Glyph glyph;
	if(ttf_render_glyph_cache(c, _fonts[i].font, &glyph) != 0) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}
	PF->init(ret)->
			add(ret, &glyph, sizeof(TTY_Glyph))->
			add(ret, glyph.cache,
					_fonts[i].font->inst.maxGlyphSize.x*
					_fonts[i].font->inst.maxGlyphSize.y);
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

	device_run(&dev, mnt_point, FS_TYPE_CHAR);
	font_dev_quit();
	return 0;
}
