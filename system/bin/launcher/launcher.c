#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <console.h>
#include <vprintf.h>
#include <x/xclient.h>
#include <sconf.h>
#include <graph/tga.h>

#define ITEM_MAX 16

typedef struct {
	int icon_size;
	int num;
	str_t* items[ITEM_MAX];	
} items_t;

static int32_t read_config(const char* fname, items_t* items) {
	sconf_t *conf = sconf_load(fname);	
	if(conf == NULL)
		return -1;
	items->icon_size = atoi(sconf_get(conf, "icon_size"));

	int i = 0;
	while(1) {
		sconf_item_t* it = sconf_get_at(conf, i++);
		if(it == NULL || it->name == NULL || it->value == NULL)
			break;
		if(strcmp(it->name->cstr, "item") == 0) {
			items->items[items->num] = str_new(it->value->cstr);
			items->num++;
			if(items->num >= ITEM_MAX)
				break;
		}
	}
	sconf_free(conf);
	return 0;
}

static void draw_icon(graph_t* g, const char* item, int icon_size, int i) {
	str_t* s = str_new("");	
	int at = str_to(item, ',', NULL, 1);
	str_to(item + at + 1, ',', s, 1);

	graph_t* img = tga_image_new(s->cstr);
	str_free(s);
	if(img == NULL)
		return;

	reverse(img);
	int dx = (icon_size - img->w)/2;
	int dy = (icon_size - img->h)/2;

	blt_alpha(img, 0, 0, img->w, img->h,
			g, dx, dy+i*icon_size, img->w, img->h, 0xff);
	graph_free(img);
}

static void draw(x_t* x, items_t* items) {
	graph_t* g = x_get_graph(x);
	//font_t* font = get_font_by_name("8x16");
	clear(g, argb_int(0x0));
	int i;
	for(i=0; i<items->num; i++) {
		/*box(g, 0, i*items->icon_size,
			items->icon_size,
			items->icon_size,
			0xffaaaaaa);
			*/
		draw_icon(g, items->items[i]->cstr, items->icon_size, i);
		//draw_text(g, 0, i*items->icon_size + 4, items->items[i], font, 0xff888888);
	}
	x_release_graph(x, g);
	x_update(x);
}

static void run(const char* item) {
	str_t* s = str_new("");	
	str_to(item, ',', s, 1);
	exec(s->cstr);
	str_free(s);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	items_t items;
	memset(&items, 0, sizeof(items_t));
	read_config("/etc/x/launcher.conf", &items);

	xscreen_t scr;
	x_screen_info(&scr);
	x_t* x = x_open(scr.size.w - items.icon_size - 24,
			24,
			items.icon_size, 
			items.icon_size * items.num,
			"launcher", X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_NO_FOCUS);
	draw(x, &items);
	x_set_visible(x, true);

	xinfo_t xinfo;
	x_get_info(x, &xinfo);
	xevent_t xev;
	while(1) {
		if(x_get_event(x, &xev) == 0) {
			if(xev.type == XEVT_MOUSE && xev.state == XEVT_MOUSE_DOWN) {
				int i = div_u32(xev.value.mouse.y - xinfo.r.y, items.icon_size);
				if(i < items.num) {
					int pid = fork();
					if(pid == 0)
						run(items.items[i]->cstr);
				}
			}
		}
		else {
			usleep(100000);
		}
	}

	int i;
	for(i=0; i<items.num; i++) {
		str_free(items.items[i]);
	}

	x_close(x);
	return 0;
} 
