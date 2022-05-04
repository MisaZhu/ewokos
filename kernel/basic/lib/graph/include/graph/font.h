#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stddef.h>

typedef struct  {
	int32_t idx;
	uint32_t w, h;
	const void *data;
	int32_t pref;
} font_t;

typedef struct {
	const char* name;
	font_t* font;
} font_item_t;

font_t font_8x16;

int32_t get_text_size(const char* s, font_t* font, int32_t *w, int32_t* h);

#endif
