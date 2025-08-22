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

static int _font_dev_pid = -1;

int font_init(void) {
	_font_dev_pid = dev_get_pid("/dev/font");
	if(_font_dev_pid < 0)
		return -1;
	return 0;
}

void font_quit(void) {

}

const char*  font_name_by_fname(const char* fname) {
	static char ret[128];
	memset(ret, 0, 128);

	char sname[FS_FULL_NAME_MAX];
	vfs_file_name(fname, sname, FS_FULL_NAME_MAX);
	strncpy(ret, sname, 127);
	int i = strlen(sname)-1;
	while(i >= 0) {
		if(ret[i] == '.') {
			ret[i] = 0;
			break;
		}
		--i;
	}
	return ret;
}

int font_load(const char* name, const char* fname) {
	if(_font_dev_pid < 0)
		return -1;

	if(name == NULL)
		name = font_name_by_fname(fname);
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "s,s", name, fname);

	int ret = -1;
	if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_LOAD, &in, &out) == 0) {
		ret = proto_read_int(&out);
	}

	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

int font_get_face(font_t* font, uint32_t size, face_info_t* face) {
	if(_font_dev_pid < 0 || font == NULL)
		return -1;
	memset(face, 0, sizeof(face_info_t));

	int ret = -1;
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i", font->id, size);

	if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_GET_FACE, &in, &out) == 0) {
		proto_read_to(&out, face, sizeof(face_info_t));
		ret = 0;
	}
	PF->clear(&in);
	PF->clear(&out);
	return ret;
}

int font_get_glyph_info(font_t* font, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
	if(_font_dev_pid < 0 || font == NULL)
		return -1;
	memset(slot, 0, sizeof(FT_GlyphSlotRec));

	int ret = -1;
	proto_t in, out;
	PF->init(&out);
	PF->format(&in, "i,i,i", font->id, size, c);

	if(dev_cntl_by_pid(_font_dev_pid, FONT_DEV_GET_GLYPH, &in, &out) == 0) {
		proto_read_to(&out, slot, sizeof(FT_GlyphSlotRec));
		face_info_t* faceinfo = malloc(sizeof(face_info_t));
		proto_read_to(&out, faceinfo, sizeof(face_info_t));
		slot->other = faceinfo;

		
		int32_t sz;
		void* p = proto_read(&out, &sz);
		if(p == NULL || sz <= 0) {
			slot->bitmap.buffer = NULL;
		}
		else {
			slot->bitmap.buffer = malloc(sz);
			memcpy(slot->bitmap.buffer, p, sz);
		}
		PF->clear(&in);
		PF->clear(&out);
		ret = 0;
	}
	return ret;
}

#ifdef __cplusplus
}
#endif