#include <graph/graph.h>
#include <kstring.h>

static void draw_char8(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h;
	ypart = y;

	for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
		xpart = x;
		pmask = 1 << (8-1);
		check = pdata[index+ychar];
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				graph_pixel_safe(g, xpart, ypart, color);
			xpart++;
			pmask >>= 1;
		}
		ypart++;
	}
}

static void draw_char16(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	int32_t xchar, ychar, xpart, ypart, index, pmask;
	unsigned char *pdata = (unsigned char*) font->data, check;

	index = (int32_t)c * font->h * 2;
	ypart = y;
	for(ychar=0; (uint32_t)ychar<font->h; ychar++) {
		xpart = x;
		pmask = 1 << (8-1);
		check = pdata[index+ychar];
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				graph_pixel_safe(g, xpart, ypart, color);
			xpart++;
			pmask >>= 1;
		}

		index++;
		check = pdata[index+ychar];
		pmask = 1 << (8-1);
		for(xchar=0; (uint32_t)xchar<8; xchar++) {
			if(check&pmask)
				graph_pixel_safe(g, xpart, ypart, color);
			xpart++;
			pmask >>= 1;
		}
		ypart++;
	}
}

void graph_draw_char(graph_t* g, int32_t x, int32_t y, char c, font_t* font, uint32_t color) {
	if(g == NULL)
		return;
	if(font->w <= 8)
		draw_char8(g, x, y, c, font, color);
	else if(font->w <= 16)
		draw_char16(g, x, y, c, font, color);
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
