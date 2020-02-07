#include "font.h"
#include "kstring.h"

extern font_t font_8x16;

static font_item_t _fonts[] ={
	{"8x16", &font_8x16},
	{"", NULL}
};

static uint32_t _font_num = 1;

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
