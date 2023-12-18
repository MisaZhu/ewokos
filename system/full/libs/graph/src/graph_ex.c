#include <graph/graph_ex.h>

void graph_get_3d_color(uint32_t base, uint32_t *dark, uint32_t *bright) {
	uint32_t a, r, g, b, c;
	a = (base >> 24) & 0xff;
	r = (base >> 16) & 0xff;
	g = (base >> 8) & 0xff;
	b = base & 0xff;

	c = r<g ? r:g;
	c = c<b ? c:b;

	*dark = argb(a, (c/3)*2, (c/3)*2, (c/3)*2);

	r = r<0xAA ? r:0xAA;
	g = g<0xAA ? g:0xAA;
	b = b<0xAA ? b:0xAA;

	c = r>g ? r:g;
	c = c>b ? c:b;
	*bright = argb(a, (c/3)*4, (c/3)*4, (c/3)*4);
}

void graph_fill_3d(graph_t* g, int x, int y, int w, int h, uint32_t color, bool rev) {
	uint32_t dark, bright;
	if(rev)
		graph_get_3d_color(color, &bright, &dark);
	else
		graph_get_3d_color(color, &dark, &bright);

	graph_fill(g, x+1, y+1, w-2, h-2, color);
	graph_box_3d(g, x, y, w, h, bright, dark);
}

void graph_box_3d(graph_t* g,
		int x, int y, int w, int h,
		uint32_t bright_color, uint32_t dark_color) {
	graph_line(g, x, y, x+w-1, y, bright_color);
	graph_line(g, x, y+1, x, y+h-1, bright_color);
	graph_line(g, x+w-1, y, x+w-1, y+h-1, dark_color);
	graph_line(g, x, y+h-1, x+w-1, y+h-1, dark_color);
}

void graph_frame(graph_t* g, int x, int y, int w, int h, int width, uint32_t base_color, bool rev) {
	uint32_t dark, bright;
	if(rev)
		graph_get_3d_color(base_color, &bright, &dark);
	else
		graph_get_3d_color(base_color, &dark, &bright);

	graph_box_3d(g, x, y, w, h, bright, dark);
	for(int i=1; i<(width-1); i++) {
		graph_box(g, x+i, y+i, w-i*2, h-i*2, base_color);
	}
	graph_box_3d(g, x+width-1, y+width-1, w-width*2+2, h-width*2+2, dark, bright);
}

void graph_draw_dot_pattern(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2, uint8_t dw) {
	int i = 0;
	int j = 0;
	bool shift = false;

	graph_fill(g, x, y, w, h, c1);
	while(j < h) {
		while(i < w) {
			//graph_pixel(g, x+i, y+j, c2);
			graph_fill(g, x+i, y+j, dw, dw, c2);
			i += 2*dw;
		}
		i = shift ? 0:dw;
		shift = !shift;
		j += dw;
	}
}

void graph_gradation(graph_t* graph, int x, int y, int w, int h, uint32_t c1, uint32_t c2, bool vertical) {
	int32_t a1 = (c1 >> 24) & 0xff;
	int32_t a2 = (c2 >> 24) & 0xff;
	int32_t r1 = (c1 >> 16) & 0xff;
	int32_t r2 = (c2 >> 16) & 0xff;
	int32_t g1 = (c1 >> 8) & 0xff;
	int32_t g2 = (c2 >> 8) & 0xff;
	int32_t b1 = c1 & 0xff;
	int32_t b2 = c2 & 0xff;

	if(!vertical) {
			for(int32_t i=1; i<=w; i++) {
				int32_t a = a1 + (((a2 - a1) * i ) / w);
				int32_t r = r1 + (((r2 - r1) * i ) / w);
				int32_t g = g1 + (((g2 - g1) * i ) / w);
				int32_t b = b1 + (((b2 - b1) * i ) / w);
				uint32_t c = (uint32_t)((a << 24) | (r << 16) | (g << 8) | b);
				graph_line(graph, x+i-1, y, x+i-1, y+h-1, c);
			}
	}
	else {
			for(int32_t i=1; i<=h; i++) {
				int32_t a = a1 + (((a2 - a1) * i ) / h);
				int32_t r = r1 + (((r2 - r1) * i ) / h);
				int32_t g = g1 + (((g2 - g1) * i ) / h);
				int32_t b = b1 + (((b2 - b1) * i ) / h);
				uint32_t c = (uint32_t)((a << 24) | (r << 16) | (g << 8) | b);
				graph_line(graph, x, y+i-1, x+w-1, y+i-1, c);
			}
	}
}