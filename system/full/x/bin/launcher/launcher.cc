#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <sys/basic_math.h>
#include <dirent.h>

#define ITEM_MAX 128
using namespace Ewok;

typedef struct {
	str_t* app;
	str_t* fname;
	str_t* icon;
	graph_t* iconImg;
} item_t;

typedef struct {
	int icon_size;
	int item_size;
	int num;
	item_t items[ITEM_MAX];	
} items_t;

class Launcher: public XWin {
	items_t items;
	int selected;
	bool focused;
	font_t* font;
	uint32_t titleColor;

	void drawIcon(graph_t* g, int at, int x, int y) {
		const char* icon = items.items[at].icon->cstr;
		int item_size = items.item_size;
		int icon_size = items.icon_size;
		graph_t* img = items.items[at].iconImg;
		if(img == NULL) {
			graph_t* i = png_image_new(icon);
			if(i == NULL)
				return;
			if(i->w != icon_size) {
				img = graph_scalef(i, ((float)icon_size) / ((float)i->w));
				graph_free(i);
			}
			else 
				img = i;
			items.items[at].iconImg = img;
		}

		int dx = (item_size - img->w)/2;
		int dy = (item_size - img->h)/2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, int x, int y) {
		const char* title = items.items[at].app->cstr;
		int item_size = items.item_size;
		int32_t w;
		get_text_size(title, font, &w, NULL);
		int dx = (item_size - w)/2;
		int dy = (item_size - font->h);
		graph_draw_text(g, x+dx, y+dy, title, font, titleColor);
	}

	void runProc(const char* app) {
		exec(app);
	}

protected:
	void onRepaint(graph_t* g) {
		//font_t* font = get_font_by_name("8x16");
		graph_clear(g, 0x0);
		int i, j, cols, rows;
		cols = g->w / items.item_size;
		rows = items.num / cols;
		if((items.num % cols) != 0)
			rows++;
			
		for(j=0; j<rows; j++) {
			for(i=0; i<cols; i++) {
				int at = j*cols + i;
				if(at >= items.num)
					return;

				int x = i*items.item_size;
				int y = j*items.item_size;
				if(focused && selected == at) {
					graph_fill_round(g, 
						x, y, items.item_size, items.item_size, 
						8, 0x88000000);
					graph_round(g, 
						x, y, items.item_size, items.item_size, 
						8, 0xffffffff);
				}

				drawIcon(g, at, x, y);
				drawTitle(g, at, x, y);
			}
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		int cols = xinfo.wsr.w / items.item_size;
		if(ev->type == XEVT_MOUSE) {
			int col = ev->value.mouse.x / items.item_size;
			int row = ev->value.mouse.y / items.item_size;
			int at = row*cols + col;
			if(at >= items.num)
				return;

			if(ev->state == XEVT_MOUSE_DOWN) {
				if(selected != at) {
					selected = at;
					repaint(true);
				}
			}
			else if(ev->state == XEVT_MOUSE_UP) {
				int pid = fork();
				if(pid == 0)
					runProc(items.items[at].fname->cstr);
				return;
			}
		}
		else if(ev->type == XEVT_IM) {
			int key = ev->value.im.value;
			if(ev->state == XIM_STATE_PRESS) {
				if(key == KEY_LEFT)
					selected--;
				else if(key == KEY_RIGHT)
					selected++;
				else if(key == KEY_UP)
					selected -= cols;
				else if(key == KEY_DOWN)
					selected += cols;
				else
					return;
			}
			else {//XIM_STATE_RELEASE
				if(key == KEY_ENTER || key == KEY_BUTTON_START) {
					int pid = fork();
					if(pid == 0) {
						runProc(items.items[selected].fname->cstr);
						exit(0);
					}
				}
				return;
			}

			if(selected >= (items.num-1))
				selected = items.num-1;
			if(selected < 0)
				selected = 0;
			repaint(true);
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
		memset(&items, 0, sizeof(items_t));
		font = font_by_name("8x16");
	}

	inline ~Launcher() {
		for(int i=0; i<items.num; i++) {
			str_free(items.items[i].app);
			str_free(items.items[i].fname);
			str_free(items.items[i].icon);
			if(items.items[i].iconImg)
				graph_free(items.items[i].iconImg);
		}
	}

	bool readConfig(const char* fname) {
		items.item_size = 96;
		items.icon_size = 64;
		titleColor = 0xffffffff;
		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		const char* v = sconf_get(conf, "icon_size");
		if(v[0] != 0)
			items.icon_size = atoi(v);
		v = sconf_get(conf, "item_size");
		if(v[0] != 0)
			items.item_size = atoi(v);
		v = sconf_get(conf, "font");
		if(v[0] != 0)
			font = font_by_name(v);
		v = sconf_get(conf, "title_color");
		if(v[0] != 0)
			titleColor = atoi_base(v, 16);
		sconf_free(conf);
		return true;
	}

	bool loadApps(void) {
		DIR* dirp = opendir("/apps");
		if(dirp == NULL)
			return false;
		int i = 0;
		while(1) {
			struct dirent* it = readdir(dirp);
			if(it == NULL || i >= ITEM_MAX)
				break;

			if(it->d_name[0] == '.')
				continue;
			items.items[i].app = str_new(it->d_name);

			items.items[i].fname = str_new("/apps/");
			str_add(items.items[i].fname, it->d_name);
			str_add(items.items[i].fname, "/");
			str_add(items.items[i].fname, it->d_name);

			items.items[i].icon = str_new("/apps/");
			str_add(items.items[i].icon, it->d_name);
			str_add(items.items[i].icon, "/res/icon.png");
			i++;
		}
		items.num = i;
		closedir(dirp);
		return true;
	}
};

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	xscreen_t scr;
	Launcher xwin;
	xwin.readConfig("/etc/x/launcher.conf");
	xwin.loadApps();

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
