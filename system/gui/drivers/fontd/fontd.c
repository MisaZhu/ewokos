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
#include "fontd.h"

ttf_item_t _ttfs[TTF_MAX];

static FT_Library _library;

static void font_dev_init(void) {
	FT_Init_FreeType(&_library);
	for(int i=0;i < TTF_MAX; i++)
		memset(&_ttfs[i], 0, sizeof(ttf_item_t));
}

static void font_dev_quit(void) {
	for(int i=0;i < TTF_MAX; i++) {
		if(_ttfs[i].face != NULL)
			FT_Done_Face(_ttfs[i].face);
	}
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
	if(i >= TTF_MAX)
		return -1;

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

static int font_dev_get_glyph(proto_t* in, proto_t* ret) {
	int i = proto_read_int(in);
	uint32_t size = (uint32_t)proto_read_int(in);
	if(i >= TTF_MAX || size == 0 || _ttfs[i].face == NULL) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	FT_Face face = _ttfs[i].face; 
    if(FT_Set_Pixel_Sizes(face, 0, size) != 0) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	uint32_t c = (uint32_t)proto_read_int(in);
    // 获取字符的字形索引
    FT_UInt glyph_index = FT_Get_Char_Index(face, c);
    // 加载并渲染字形
    if(FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER) != 0) {
		PF->init(ret)->addi(ret, -1);
		return -1;
	}

	face_info_t faceinfo;
	faceinfo.ascender = face->size->metrics.ascender;
	faceinfo.descender = face->size->metrics.descender;
	faceinfo.height = face->size->metrics.ascender - face->size->metrics.descender;

	FT_GlyphSlot slot = face->glyph;
	uint32_t bmp_size = slot->bitmap.width * slot->bitmap.rows;
	if(bmp_size > 0) {
		PF->format(ret, "m,m,m",
				slot, sizeof(FT_GlyphSlotRec),
				&faceinfo, sizeof(face_info_t),
				slot->bitmap.buffer, bmp_size);
	}
	else {
		PF->init(ret)->
				add(ret, slot, sizeof(FT_GlyphSlotRec))->
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
	PF->format(ret, "m", &faceinfo, sizeof(face_info_t));
	return 0;
}

static int font_dev_list(proto_t* in, proto_t* ret) {
	uint32_t i = 0;
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

int main(int argc, char** argv) {
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
