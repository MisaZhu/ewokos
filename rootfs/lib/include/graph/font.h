#ifndef FONT_H
#define FONT_H

#include <types.h>

typedef struct  {
	int32_t idx;
	uint32_t w, h;
	const void *data;
	int32_t pref;
} font_t;

font_t* get_font(const char* name);

#endif
