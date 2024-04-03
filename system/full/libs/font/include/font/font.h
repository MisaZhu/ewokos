#ifndef FONT_H 
#define FONT_H

#include <stdint.h>
#include <ttf/ttf.h>
#include <ewoksys/hashmap.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_SYSTEM_FONT       "system"
#define DEFAULT_SYSTEM_FONT_FILE  "/usr/system/fonts/system.ttf"
#define DEFAULT_SYSTEM_FONT_SIZE  12

enum {
    FONT_DEV_LOAD = 0,
    FONT_DEV_CLOSE,
    FONT_DEV_GET,
    FONT_DEV_LIST
};

#define FONT_ALIGN_NONE    0x00
#define FONT_ALIGN_CENTER  0x01

typedef struct {
    int id;
    uint16_t ppm;
    TTY_V2 max_size;
	map_t *cache;
} font_inst_t;

#define FONT_NAME_MAX 64
#define FONT_INST_MAX 8

typedef struct {
    char name[FONT_NAME_MAX];
    font_inst_t instances[FONT_INST_MAX];
} font_t;


int     font_init(void);
int     font_close(font_t* font);

font_t* font_new(const char* fname, bool safe);
int     font_free(font_t* font);

TTY_Glyph* font_init_glyph(TTY_Glyph* glyph); 

void font_char_size(uint16_t c, font_t* font, uint16_t size, uint16_t *w, uint16_t* h);
void font_text_size(const char* str, font_t* font , uint16_t size, uint32_t *w, uint32_t* h);

void graph_draw_char_font(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint16_t size, uint32_t color, TTY_U16* w, TTY_U16* h);
void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint16_t size, uint32_t color, TTY_U16 w, TTY_U16 h);
void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
		font_t* font, uint16_t size, uint32_t color);
void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
        const char* str, font_t* font, uint16_t size, uint32_t color, uint32_t align);

#ifdef __cplusplus
}
#endif
#endif