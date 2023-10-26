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
	graph_box_3d(g, x+width-1, y+width-1, w-width-2, h-width-2, dark, bright);
}

void graph_draw_dot_pattern(graph_t* g,int x, int y, int w, int h, uint32_t c1, uint32_t c2) {
	int i = 0;
	int j = 0;
	bool shift = false;

	graph_fill(g, x, y, w, h, c1);
	while(j < h) {
		while(i < w) {
			graph_pixel(g, x+i, y+j, c2);
			i += 2;
		}
		i = shift ? 0:1;
		shift = !shift;
		j++;
	}
}