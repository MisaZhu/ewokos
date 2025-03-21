#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stddef.h>

typedef struct  {
	int32_t idx;
	uint32_t w, h;
	int32_t pref;
	int (*get_pixel)(char c, int x, int y);
} font_t;

typedef struct {
	const char* name;
	font_t* font;
} font_item_t;


int32_t get_text_size(const char* s, font_t* font, int32_t *w, int32_t* h);
font_t* get_font(uint32_t size);

#endif
