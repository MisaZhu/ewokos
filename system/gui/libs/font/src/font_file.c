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
int font_init(void) {
	FT_Init_FreeType(&_library);
	for(int i=0;i < TTF_MAX; i++)
		memset(&_ttfs[i], 0, sizeof(ttf_item_t));
	return 0;
}

void font_quit() {
	for(int i=0;i < TTF_MAX; i++) {
		if(_ttfs[i].face != NULL) {
			FT_Done_Face(_ttfs[i].face);
			_ttfs[i].face = NULL;
		}
	}
	FT_Done_FreeType(&_library);
}

int font_load(const char* name, const char* fname) {
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

	char font_fname[FNAME_LEN] = {0};
	if(fname == NULL || fname[0] == 0) {
		snprintf(font_fname, FNAME_LEN-1, "%s/%s.ttf", DEFAULT_SYSTEM_FONT_PATH, name);
	}
	else {
		strncpy(font_fname, fname, FNAME_LEN-1);
	}

	if(FT_New_Face(_library, font_fname, 0, &_ttfs[i].face) != 0)
		return -1;
	strncpy(_ttfs[i].name, name, NAME_LEN-1);
	strncpy(_ttfs[i].fname, font_fname, FNAME_LEN-1);
	return i;
}

static int load_glyph_rendered(FT_Face face, FT_UInt glyph_index) {
	if(FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_TARGET_NORMAL) == 0) {
		if(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL) == 0)
			return 0;
	}
	if(FT_Load_Glyph(face, glyph_index,
			FT_LOAD_DEFAULT | FT_LOAD_NO_HINTING | FT_LOAD_TARGET_NORMAL) != 0)
		return -1;
	return FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
}

static uint32_t bitmap_row_bytes(const FT_Bitmap* bmp) {
	if(bmp == NULL || bmp->pitch == 0)
		return 0;
	return (uint32_t)(bmp->pitch < 0 ? -bmp->pitch : bmp->pitch);
}

int font_get_face(font_t* font, uint32_t size, face_info_t* faceinfo) {
	int i = font->id;
	if(i >= TTF_MAX || size == 0 || _ttfs[i].face == NULL) {
		return -1;
	}

	FT_Face face = _ttfs[i].face; 
    if(FT_Set_Pixel_Sizes(face, 0, size) != 0) {
		return -1;
	}

	faceinfo->ascender = face->size->metrics.ascender;
	faceinfo->descender = face->size->metrics.descender;
	faceinfo->height = face->size->metrics.ascender - face->size->metrics.descender;
	faceinfo->width = face->size->metrics.max_advance;
	return 0;
}

int font_get_glyph_info(font_t* font, uint32_t size, uint32_t c, FT_GlyphSlot slot) {
	if(font == NULL)
		return -1;
	memset(slot, 0, sizeof(FT_GlyphSlotRec));

	int findex = font->id;
	if(findex >= TTF_MAX || size == 0 || _ttfs[findex].face == NULL) {
		return -1;
	}

	FT_Face face = _ttfs[findex].face; 
	//if(_ttfs[findex].current_size != size) {
		if(FT_Set_Pixel_Sizes(face, 0, size) != 0) {
			return -1;
		}
		//_ttfs[findex].current_size = size;
	//}

	FT_UInt glyph_index = FT_Get_Char_Index(face, c);
	// If character not found in font, try to use default character (usually space or replacement char)
	if(glyph_index == 0) {
		// Try to get default glyph (index 0 is usually the missing glyph)
		// For spaces and control characters, use a default width
		if(c == ' ' || c == '\t' || c == '\n' || c == '\r') {
			// Space character - use half of max_advance as default space width
			face_info_t* faceinfo = malloc(sizeof(face_info_t));
			faceinfo->ascender = face->size->metrics.ascender;
			faceinfo->descender = face->size->metrics.descender;
			faceinfo->height = face->size->metrics.ascender - face->size->metrics.descender;
			// Space width is typically half of max_advance or based on font design
			FT_Long space_width = face->size->metrics.max_advance / 2;
			if(space_width == 0) space_width = size * 32; // Fallback: size/2 in 26.6 format
			faceinfo->width = space_width;
			slot->other = faceinfo;
			slot->metrics.horiAdvance = space_width;
			return 0;
		}
	}
	
	// Use FT_LOAD_TARGET_NORMAL for anti-aliased rendering (gray scale)
	if(load_glyph_rendered(face, glyph_index) != 0) {
		return -1;
	}
	memcpy(slot, face->glyph, sizeof(FT_GlyphSlotRec));
	face_info_t* faceinfo = malloc(sizeof(face_info_t));
	uint32_t bmp_size = bitmap_row_bytes(&face->glyph->bitmap) * slot->bitmap.rows;
	if(bmp_size > 0) {
		slot->bitmap.buffer = malloc(bmp_size);
		memcpy(slot->bitmap.buffer, face->glyph->bitmap.buffer, bmp_size);
	}

	faceinfo->ascender = face->size->metrics.ascender;
	faceinfo->descender = face->size->metrics.descender;
	faceinfo->height = face->size->metrics.ascender - face->size->metrics.descender;
	faceinfo->width = face->size->metrics.max_advance;
	slot->other = faceinfo;
	return 0;
}

#ifdef __cplusplus
}
#endif
