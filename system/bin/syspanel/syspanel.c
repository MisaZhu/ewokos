#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <sconf.h>
#include <tga/tga.h>
#include <graph/graph.h>

#define ITEM_MAX 16

typedef struct {
	font_t* font;
} items_t;

static items_t _items;

static int32_t read_config(const char* fname, items_t* items) {
	items->font = font_by_name("8x16");
	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;
	items->font = font_by_name(sconf_get(conf, "font"));
	sconf_free(conf);
	return 0;
}

static void repaint(x_t* x, graph_t* g) {
	(void)x;
	clear(g, argb_int(0xffaaaaaa));
	draw_text(g, 2, 2, "EwokOS", _items.font, 0xff222222);
	line(g, 0, g->h-1, g->w, g->h-1, argb_int(0xff222222));
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	memset(&_items, 0, sizeof(items_t));
	read_config("/etc/x/syspanel.conf", &_items);

	xscreen_t scr;
	x_screen_info(&scr);
	x_t* x = x_open(0,  0, scr.size.w, _items.font->h + 4,
			"syspanel", X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_NO_FOCUS);

	x->on_repaint = repaint;

	x_set_visible(x, true);
	x_run(x);
	x_close(x);
	return 0;
}
