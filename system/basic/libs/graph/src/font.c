#include <graph/font.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t get_text_size(const char* s, font_t* font, int32_t* w, int32_t* h) {
	if(font == NULL)
		return -1;
	if(w != NULL)
		*w = strlen(s) * font->w;
	if(h != NULL)
		*h = font->h;
	return 0;
}

#ifdef __cplusplus 
}
#endif

