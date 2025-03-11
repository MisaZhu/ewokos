#include <graph/graph.h>
#include <kstring.h>

void graph_draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	uint32_t xchar, ychar;
	if(g == NULL || font->get_pixel == NULL)
		return;

	for(xchar = 0; xchar < font->w; xchar++) {
		for(ychar = 0; ychar < font->h; ychar++){
			if(font->get_pixel(c, xchar, ychar)){
				graph_pixel_safe(g, x + xchar, y + ychar, color);
			}
		}
	}
	// if(font->w == 8)
	// 	draw_char8(g, x, y, c, font, color);
	// else if(font->w == 16)
	// 	draw_char16(g, x, y, c, font, color);
	// else if(font->w == 5)
	// 	draw_char5(g, x, y, c, font, color);
}

void graph_draw_text(graph_t* g, int32_t x, int32_t y, const char* str, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	int32_t ox = x;
	while(*str) {
		if(*str == '\n') {
			x = ox;
			y += font->h;
		}
		else {
			graph_draw_char(g, x, y, *str, font, color);
			x += font->w;
		}
		str++;
	}
}
