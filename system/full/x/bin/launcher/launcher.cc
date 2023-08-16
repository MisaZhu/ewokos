#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <font/font.h>
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
	int rows;
	int cols;
	int margin;
	int num;
	item_t items[ITEM_MAX];	
} items_t;

class Launcher: public XWin {
	items_t items;
	int selected;
	int start;
	bool focused;
	font_t font;
	uint32_t titleColor;
	uint32_t bgColor;
	uint32_t iconBGColor;
	uint32_t selectedColor;
	uint32_t fontSize;
	uint32_t titleMargin;
	uint32_t height;

	void drawIcon(graph_t* g, int at, int x, int y, int w, int h) {
		const char* icon = items.items[at].icon->cstr;
		int icon_size = items.icon_size < w ? items.icon_size : w;
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

		int dx = (w - img->w)/2;
		int dy = (h - (int)(items.icon_size + titleMargin + fontSize)) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, int x, int y, int w, int h) {
		const char* title = items.items[at].app->cstr;
		uint32_t tw, th;
		font_text_size(title, &font, &tw, &th);
		int dx = (w - tw)/2;
		int dy = (h - (int)(items.icon_size + titleMargin + th)) /2 +
				items.icon_size + titleMargin;
		graph_draw_text_font(g, x+dx, y+dy, title, &font, titleColor);
	}

	void runProc(const char* app) {
		exec(app);
	}

protected:
	void onRepaint(graph_t* g) {
		graph_clear(g, bgColor);
		int i, j, itemH, itemW;
		//cols = g->w / items.item_size;
		//rows = items.num / cols;
		itemH = (g->h - items.margin) / items.rows;
		itemW = (g->w - items.margin) / items.cols;
		//if((items.num % cols) != 0)
		//	rows++;
			
		for(j=0; j<items.rows; j++) {
			for(i=0; i<items.cols; i++) {
				int at = j*items.cols + i + start;
				if(at >= items.num)
					return;

				int x = i*itemW + items.margin;
				int y = j*itemH + items.margin;
				graph_set_clip(g, x, y, itemW-items.margin, itemH-items.margin);
				if(selected == at && focused) {
					graph_fill_round(g, 
							x, y, itemW-items.margin, itemH-items.margin, 
							8, selectedColor);
				}
				else {
					graph_fill_round(g, 
							x, y, itemW-items.margin, itemH-items.margin, 
							8, iconBGColor);
				}

				drawIcon(g, at, x, y, itemW-items.margin, itemH-items.margin-titleMargin);
				drawTitle(g, at, x, y, itemW-items.margin, itemH-items.margin);
			}
		}
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		if(ev->type == XEVT_MOUSE) {
			int itemW = xinfo.wsr.w / items.cols;
			int itemH = xinfo.wsr.h / items.rows;
			int col = (ev->value.mouse.x - xinfo.wsr.x) / itemW;
			int row = (ev->value.mouse.y - xinfo.wsr.y) / itemH;
			int at = row*items.cols + col + start;
			if(at >= items.num)
				return;

			if(ev->state == XEVT_MOUSE_DOWN) {
				if(selected != at) {
					selected = at;
					repaint();
				}
			}
			else if(ev->state == XEVT_MOUSE_CLICK) {
				int pid = fork();
				if(pid == 0) {
					runProc(items.items[at].fname->cstr);
					exit(0);
				}
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
					selected -= items.cols;
				else if(key == KEY_DOWN)
					selected += items.cols;
				else
					return;
			}
			else {//XIM_STATE_RELEASE
				if(key == KEY_ENTER || key == KEY_BUTTON_START || key == KEY_BUTTON_A) {
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
			
			if(selected < start) {
				start -= items.cols*items.rows;
				if(start < 0)
					start = 0;
			}
			else if((selected - start) >= items.cols*items.rows) 
				start += items.cols*items.rows;
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
		height = 0;
		titleMargin = 2;
		selected = 0;
		start = 0;
		focused = true;
		memset(&items, 0, sizeof(items_t));
	}

	inline ~Launcher() {
		for(int i=0; i<items.num; i++) {
			str_free(items.items[i].app);
			str_free(items.items[i].fname);
			str_free(items.items[i].icon);
			if(items.items[i].iconImg)
				graph_free(items.items[i].iconImg);
		}
		if(font.id >= 0)
			font_close(&font);
	}

	bool readConfig(const char* fname) {
		items.cols = 4;
		items.rows = 2;
		items.margin = 6;
		items.icon_size = 64;
		titleColor = 0xffffffff;
		bgColor = 0xff000000;
		iconBGColor = 0x88aaaaaa;
		selectedColor = 0x88444444;
		height = 0;
		fontSize = 14;

		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		const char* v = sconf_get(conf, "icon_size");
		if(v[0] != 0)
			items.icon_size = atoi(v);
		v = sconf_get(conf, "rows");
		if(v[0] != 0)
			items.rows = atoi(v);

		v = sconf_get(conf, "cols");
		if(v[0] != 0)
			items.cols = atoi(v);

		v = sconf_get(conf, "margin");
		if(v[0] != 0)
			items.margin = atoi(v);

		v = sconf_get(conf, "font_size");
		if(v[0] != 0)
			fontSize = atoi(v);

		v = sconf_get(conf, "font");
		if(v[0] == 0)
			v = "/data/fonts/system.ttf";
		font_load(v, fontSize, &font);

		v = sconf_get(conf, "title_color");
		if(v[0] != 0)
			titleColor = atoi_base(v, 16);

		v = sconf_get(conf, "bg_color");
		if(v[0] != 0)
			bgColor = atoi_base(v, 16);

		v = sconf_get(conf, "icon_bg_color");
		if(v[0] != 0)
			iconBGColor = atoi_base(v, 16);

		v = sconf_get(conf, "icon_selected_color");
		if(v[0] != 0)
			selectedColor = atoi_base(v, 16);

		v = sconf_get(conf, "height");
		if(v[0] != 0)
			height = atoi(v);
		else	
			height = (fontSize + items.icon_size + titleMargin + items.margin) *
					items.rows + items.margin + 6;

		sconf_free(conf);
		return true;
	}

	inline uint32_t getHeight() {
		return height;
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

	uint32_t h = xwin.getHeight();
	if(h == 0)
		h = scr.size.h;
	x.open(&xwin, 0,
			scr.size.h - h,
			scr.size.w, 
			h,
			"launcher",
			X_STYLE_NO_FRAME | X_STYLE_ALPHA | X_STYLE_LAUNCHER | X_STYLE_SYSBOTTOM);

	xwin.setVisible(true);

	x.run(NULL);
	return 0;
}
