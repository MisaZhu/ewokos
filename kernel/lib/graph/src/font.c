#include <graph/font.h>
#include <kstring.h>

int32_t get_text_size(const char* s, font_t* font, int32_t* w, int32_t* h) {
	if(font == NULL)
		return -1;
	if(w != NULL)
		*w = strlen(s) * font->w;
	if(h != NULL)
		*h = font->h;
	return 0;
}

extern font_t font_8x16;
extern font_t font_5x7;
font_t* get_font(void) {
	return &font_5x7;
}