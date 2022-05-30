#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <sys/keydef.h>
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
	int selected;
	bool focused;
	bool enter;

	void drawIcon(graph_t* g, const char* item, int icon_size, int x, int y) {
		str_t* s = str_new("");	
		int at = str_to(item, ',', NULL, 1);
		str_to(item + at + 1, ',', s, 1);

		graph_t* img = png_image_new(s->cstr);
		str_free(s);
		if(img == NULL)
			return;

		int dx = (icon_size - img->w)/2;
		int dy = (icon_size - img->h)/2;

		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
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
		int i, j, cols, rows;
		cols = g->w / items.icon_size;
		rows = items.num / cols;
		if((items.num % cols) != 0)
			rows++;
			
		for(j=0; j<rows; j++) {
			for(i=0; i<cols; i++) {
				int at = j*cols + i;
				if(at >= items.num)
					return;

				int x = i*items.icon_size;
				int y = j*items.icon_size;
				if(focused && selected == at) {
					graph_fill_round(g, 
						x, y, items.icon_size, items.icon_size, 
						8, 0x88000000);
					graph_round(g, 
						x, y, items.icon_size, items.icon_size, 
						8, 0xffffffff);
				}

				drawIcon(g, items.items[at]->cstr, items.icon_size, x, y);
			}
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		int cols = xinfo.wsr.w / items.icon_size;
		if(ev->type == XEVT_MOUSE) {
			int col = ev->value.mouse.x / items.icon_size;
			int row = ev->value.mouse.y / items.icon_size;
			int at = row*cols + col;
			if(at >= items.num)
				return;

			if(ev->state == XEVT_MOUSE_DOWN) {
				if(selected != at) {
					selected = at;
					repaint();
				}
			}
			else if(ev->state == XEVT_MOUSE_UP) {
				int pid = fork();
				if(pid == 0)
					runProc(items.items[at]->cstr);
			}
		}
		else if(ev->type == XEVT_IM) {
			int key = ev->value.im.value;
			if(key == KEY_LEFT)
				selected--;
			else if(key == KEY_RIGHT)
				selected++;
			else if(key == KEY_UP)
				selected -= cols;
			else if(key == KEY_DOWN)
				selected += cols;
			else if(key == KEY_ENTER) {
				enter = true;
				return;
			}
			else if(key == 0) {
				if(enter) {
					enter = false;
					int pid = fork();
					if(pid == 0)
						runProc(items.items[selected]->cstr);
					return;
				}
			}
			else
				return;

			if(selected >= (items.num-1))
				selected = items.num-1;
			if(selected < 0)
				selected = 0;
			repaint();
		}
	}
	
	void onFocus(void) {
		focused = true;
		repaint();
	}

	void onUnfocus(void) {
		focused = false;
		repaint();
	}

public:
	inline Launcher() {
		selected = 0;
		focused = true;
		enter = false;
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

	X x;
	x.screenInfo(scr, 0);
	x.open(&xwin, 0,
			0,
			scr.size.w, 
			scr.size.h, 
			"launcher",
			X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_LAUNCHER | X_STYLE_SYSBOTTOM);

	xwin.setVisible(true);

	x.run(NULL);
	return 0;
}
