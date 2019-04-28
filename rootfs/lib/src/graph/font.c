#include "graph/font.h"
#include "kstring.h"

extern font_t font_16x32;
extern font_t font_12x24;
extern font_t font_10x20;
extern font_t font_12x16;
extern font_t font_9x16;
extern font_t font_8x16;
extern font_t font_8x10;
extern font_t font_8x8;
extern font_t font_7x9;
extern font_t font_5x12;

font_t* get_font(const char* name) {
	if(strcmp(name, "16x32") == 0)
		return &font_16x32;
	else if(strcmp(name, "12x24") == 0)
		return &font_12x24;
	else if(strcmp(name, "10x20") == 0)
		return &font_10x20;
	else if(strcmp(name, "12x16") == 0)
		return &font_12x16;
	else if(strcmp(name, "9x16") == 0)
		return &font_9x16;
	else if(strcmp(name, "8x16") == 0)
		return &font_8x16;
	else if(strcmp(name, "8x10") == 0)
		return &font_8x10;
	else if(strcmp(name, "8x8") == 0)
		return &font_8x8;
	else if(strcmp(name, "7x9") == 0)
		return &font_7x9;
	else if(strcmp(name, "5x12") == 0)
		return &font_5x12;
	return NULL;
}
