#include "graph/font.h"
#include "kstring.h"

extern font_t font_16x32;
extern font_t font_12x24;
extern font_t font_10x20;
extern font_t font_12x16;
extern font_t font_9x16;
extern font_t font_8x16;
extern font_t font_8x10;
extern font_t font_9x8;
extern font_t font_8x8;
extern font_t font_7x9;
extern font_t font_6x8;
extern font_t font_5x12;
extern font_t font_4x6;

static font_item_t _fonts[] ={
	{"16x32", &font_16x32},
	{"12x24", &font_12x24},
	{"10x20", &font_10x20},
	{"12x16", &font_12x16},
	{"9x16", &font_9x16},
	{"8x16", &font_8x16},
	{"8x10", &font_8x10},
	{"9x8", &font_9x8},
	{"8x8", &font_8x8},
	{"7x9", &font_7x9},
	{"6x8", &font_6x8},
	{"5x12", &font_5x12},
	{"4x6", &font_4x6},
	{"", NULL}
};

static uint32_t _font_num = 14;

font_t* font_by_name(const char* name) {
	int32_t i = 0;
	while(1) {
		if(_fonts[i].name[0] == 0)
			break;
		if(strcmp(_fonts[i].name, name) == 0)
			return _fonts[i].font;
		i++;
	}
	return NULL;
}

font_item_t* font_by_index(uint32_t index) {
	if(index >= _font_num)
		return NULL;
	return &_fonts[index];
}
