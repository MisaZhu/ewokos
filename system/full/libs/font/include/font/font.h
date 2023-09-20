#ifndef FONT_H 
#define FONT_H

#include <stdint.h>
#include <ttf/ttf.h>
#include <hashmap.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    FONT_DEV_LOAD = 0,
    FONT_DEV_CLOSE,
    FONT_DEV_GET
};

#define FONT_ALIGN_NONE    0x00
#define FONT_ALIGN_CENTER  0x01

typedef struct {
    int id;
    TTY_V2 max_size;
	map_t *cache;
} font_t;


int font_init(void);
int font_load(const char* fname, uint16_t ppm, font_t* font);
int font_close(font_t* font);
int font_get_glyph(font_t* font, uint16_t c, TTY_Glyph* glyph); 

TTY_Glyph* font_init_glyph(TTY_Glyph* glyph); 

void font_char_size(uint16_t c, font_t* font,
        uint16_t *w, uint16_t* h);
void font_text_size(const char* str, font_t* font, uint32_t *w, uint32_t* h);

void graph_draw_char_font(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint32_t color, TTY_U16* w, TTY_U16* h);
void graph_draw_char_font_fixed(graph_t* g, int32_t x, int32_t y, TTY_U32 c,
		font_t* font, uint32_t color, TTY_U16 w, TTY_U16 h);
void graph_draw_text_font(graph_t* g, int32_t x, int32_t y, const char* str,
		font_t* font, uint32_t color);
void graph_draw_text_font_align(graph_t* g, int32_t x, int32_t y, int32_t w, int32_t h,
        const char* str, font_t* font, uint32_t color, uint32_t align);


#ifdef __cplusplus
}
#endif
#endif