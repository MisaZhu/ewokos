#ifndef FONT_H
#define FONT_H

#include <stdint.h>

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

font_t* font_by_name(const char* name);
font_item_t* font_by_index(uint32_t index);

int32_t get_text_size(const char* s, font_t* font, uint32_t *w, uint32_t* h);

#endif
