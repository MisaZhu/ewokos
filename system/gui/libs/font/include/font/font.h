#ifndef FONT_H 
#define FONT_H

#include <stdint.h>
#include <freetype/freetype.h>
#include <ewoksys/hashmap.h>
#include <graph/graph.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SYSTEM_FONT       "system"
#define DEFAULT_SYSTEM_FONT_PATH  "/usr/system/fonts"
#define DEFAULT_SYSTEM_FONT_FILE  "/usr/system/fonts/system.ttf"
#define DEFAULT_SYSTEM_FONT_SIZE  12

enum {
    FONT_DEV_LOAD = 0,
    FONT_DEV_GET_GLYPH,
    FONT_DEV_GET_FACE,
    FONT_DEV_LIST
};

#define FONT_ALIGN_NONE    0x00
#define FONT_ALIGN_CENTER  0x01

#define FONT_NAME_MAX 64
#define FONT_INST_MAX 8

typedef struct {
    int16_t  ascender;
    int16_t  descender;
    uint32_t height;
    uint32_t width;
} face_info_t;

typedef struct {
    char name[FONT_NAME_MAX];
	map_t cache;
    int32_t id;
} font_t;


int     font_init(void);
void    font_quit(void);
int     font_close(font_t* font);

font_t* font_new(const char* fname, bool safe);
int     font_load(const char* name, const char* fname);
int     font_free(font_t* font);

const char*  font_name_by_fname(const char* fname);

int  font_get_height(font_t* font, uint32_t size);
int  font_get_width(font_t* font, uint32_t size);
int  font_get_face(font_t* font, uint32_t size, face_info_t* face);
int  font_get_glyph_info(font_t* font, uint32_t size, uint32_t c, FT_GlyphSlot slot);
void font_char_size(uint32_t c, font_t* font, uint32_t size, uint32_t *w, uint32_t* h);
void font_text_size(const char* str, font_t* font , uint32_t size, uint32_t *w, uint32_t* h);

void graph_draw_unicode_font(graph_t* g, int32_t x, int32_t y, uint32_t c,
		font_t* font, uint32_t size, uint32_t color, uint32_t* w, uint32_t* h);
void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, uint32_t c,
		font_t* font, uint32_t size, uint32_t color, uint32_t w, uint32_t h);
void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
		font_t* font, uint32_t size, uint32_t color);
void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
        const char* str, font_t* font, uint32_t size, uint32_t color, uint32_t align);

#ifdef __cplusplus
}
#endif
#endif