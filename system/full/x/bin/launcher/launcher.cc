#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <sys/basic_math.h>

#define ITEM_MAX 16

using namespace Ewok;

typedef struct {
	int icon_size;
	int num;
	str_t* items[ITEM_MAX];	
} items_t;

class Launcher: public XWin {
	items_t items;

	void drawIcon(graph_t* g, const char* item, int icon_size, int i) {
		str_t* s = str_new("");	
		int at = str_to(item, ',', NULL, 1);
		str_to(item + at + 1, ',', s, 1);

		graph_t* img = png_image_new(s->cstr);
		str_free(s);
		if(img == NULL)
			return;

		int dx = (icon_size - img->w)/2;
		int dy = (icon_size - img->h)/2;

		graph_blt(img, 0, 0, img->w, img->h,
				g, dx, dy+i*icon_size, img->w, img->h);
		graph_free(img);
	}

	void runProc(const char* item) {
		str_t* s = str_new("");	
		str_to(item, ',', s, 1);
		exec(s->cstr);
		str_free(s);
	}

protected:
	void onRepaint(graph_t* g) {
		//font_t* font = get_font_by_name("8x16");
		graph_clear(g, 0x0);
		int i;
		for(i=0; i<items.num; i++) {
			drawIcon(g, items.items[i]->cstr, items.icon_size, i);
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		if(ev->type == XEVT_MOUSE && ev->state == XEVT_MOUSE_UP) {
			int i = div_u32(ev->value.mouse.y - xinfo.wsr.y, items.icon_size);
			if(i < items.num) {
				int pid = fork();
				if(pid == 0)
					runProc(items.items[i]->cstr);
			}
		}
	}

public:
	inline Launcher() {
		memset(&items, 0, sizeof(items_t));
	}

	inline ~Launcher() {
		for(int i=0; i<items.num; i++) {
			str_free(items.items[i]);
		}
	}

	inline uint32_t getIconSize(void) {
		return items.icon_size;
	}
	inline uint32_t getItemNum(void) {
		return items.num;
	}

	bool readConfig(const char* fname) {
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		items.icon_size = atoi(sconf_get(conf, "icon_size"));

		int i = 0;
		while(1) {
			sconf_item_t* it = sconf_get_at(conf, i++);
			if(it == NULL || it->name == NULL || it->value == NULL)
				break;
			if(strcmp(it->name->cstr, "item") == 0) {
				items.items[items.num] = str_new(it->value->cstr);
				items.num++;
				if(items.num >= ITEM_MAX)
					break;
			}
		}
		sconf_free(conf);
		return true;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;


	xscreen_t scr;
	Launcher xwin;
	xwin.readConfig("/etc/x/launcher.conf");
	uint32_t is = xwin.getIconSize();

	X x;
	x.screenInfo(scr, 0);
	x.open(&xwin, 10,
			10,
			is, 
			is * xwin.getItemNum(),
			"launcher", X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_SYSBOTTOM | X_STYLE_NO_FOCUS);

	xwin.setVisible(true);

	x.run(NULL);
	return 0;
}
