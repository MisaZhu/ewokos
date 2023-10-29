#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <sys/keydef.h>
#include <sys/klog.h>
#include <sys/proc.h>
#include <font/font.h>
#include <dirent.h>

#define ITEM_MAX 128
using namespace Ewok;

typedef struct {
	str_t* app;
	str_t* fname;
	str_t* icon;
	graph_t* iconImg;
	int runPid;
	uint32_t runPidUUID;
} item_t;

typedef struct {
	int icon_size;
	int rows;
	int cols;
	int marginH;
	int marginV;
	int num;
	item_t items[ITEM_MAX];	
} items_t;

class Launcher: public XWin {
	const uint8_t POS_BOTTOM = 0;
	const uint8_t POS_TOP = 1;
	const uint8_t POS_LEFT = 2;
	const uint8_t POS_RIGHT = 3;

	items_t items;
	int selected;
	int start;
	bool focused;
	font_t font;
	uint32_t titleColor;
	uint32_t bgColor;
	uint32_t selectedColor;
	uint32_t fontSize;
	int32_t titleMargin;
	int32_t height;
	int32_t width;
	uint8_t position;

	void drawRunning(graph_t* g, int x, int y) {
		uint32_t mV = items.marginV/2;
		uint32_t mH = items.marginH/2;
		uint32_t r = mV>mH ? mH:mV;
		graph_fill_circle(g, x+2, y+2, r, 0x88000000);
		graph_fill_circle(g, x, y, r-1, 0xff000000);
		graph_circle(g, x, y, r, 0xffffffff);
	}

	void drawIcon(graph_t* g, int at, int x, int y, int w, int h) {
		item_t* item = &items.items[at];
		const char* icon = item->icon->cstr;
		int icon_size = items.icon_size < w ? items.icon_size : w;
		graph_t* img = item->iconImg;
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
			item->iconImg = img;
		}

		int dx = (w - img->w)/2;
		int dy = (h - (int)(items.icon_size + titleMargin + fontSize)) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
		
		if(item->runPid > 0)
			drawRunning(g, x+dx, y+dy);
	}

	void drawTitle(graph_t* g, int at, int x, int y, int w, int h) {
		const char* title = items.items[at].app->cstr;
		uint32_t tw, th;
		font_text_size(title, &font, &tw, &th);
		x += (w - (int32_t)tw)/2;
		y += (h - (int)(items.icon_size + titleMargin + (int32_t)th)) /2 +
				items.icon_size + titleMargin;
		graph_draw_text_font(g, x, y, title, &font, titleColor);
	}

	void runProc(item_t* item) {
		if(item->runPid > 0) {
			if(proc_check_uuid(item->runPid, item->runPidUUID) == item->runPidUUID) {
				x_set_top(item->runPid);
				return;
			}
		}

		int pid = fork();
		if(pid == 0)
			exec(item->fname->cstr);
		else {
			item->runPid = pid;
			item->runPidUUID = proc_get_uuid(pid);
		}
	}

protected:
	void drawItem(graph_t* g, int at, int x, int y, int w, int h) {
		graph_set_clip(g, x - items.marginH / 2, y - items.marginV / 2,
				w+items.marginH, h+items.marginV);

		if (selected == at && focused) {
			graph_fill_round(g, x - items.marginH / 2, y - items.marginV / 2,
							w+items.marginH, h+items.marginV,
							8, selectedColor);
		}
		drawIcon(g, at, x, y, w, h);
		drawTitle(g, at, x, y, w, h);
	}

	void onRepaint(graph_t* g) {
		graph_clear(g, bgColor);
		if(color_a(bgColor) != 0xff)
			setAlpha(true);
		else
			setAlpha(false);

		int i, j, itemH, itemW;
		//cols = g->w / items.item_size;
		//rows = items.num / cols;
		itemH = (g->h - items.marginV) / items.rows;
		itemW = (g->w - items.marginH) / items.cols;
		//if((items.num % cols) != 0)
		//	rows++;
			
		if(position == POS_TOP || position == POS_BOTTOM) {
			for(j=0; j<items.rows; j++) {
				for(i=0; i<items.cols; i++) {
					int at = j*items.cols + i + start;
					if(at >= items.num)
						return;
					int x = i*itemW + items.marginH;
					int y = j*itemH + items.marginV;
					drawItem(g, at, x, y, itemW-items.marginH, itemH-items.marginV);
				}
			}
		}
		else {
			for(i=0; i<items.cols; i++) {
				for(j=0; j<items.rows; j++) {
					int at = i*items.rows + j + start;
					if(at >= items.num)
						return;
					int x;
					if(position == POS_RIGHT)
						x  = (items.cols-i-1)*itemW + items.marginH;
					else
						x  = i*itemW + items.marginH;
					int y = j*itemH + items.marginV;
					drawItem(g, at, x, y, itemW-items.marginH, itemH-items.marginV);
				}
			}
		}
	}

	void onIM(xevent_t* ev) {
		int key = ev->value.im.value;
		if(ev->state == XIM_STATE_PRESS) {
			if(position == POS_TOP || position == POS_BOTTOM) {
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
			else if(position == POS_LEFT) {
				if(key == KEY_LEFT)
					selected -= items.rows;
				else if(key == KEY_RIGHT)
					selected += items.rows;
				else if(key == KEY_UP)
					selected--;
				else if(key == KEY_DOWN)
					selected++;
				else
					return;
			}
			else {
				if(key == KEY_LEFT)
					selected += items.rows;
				else if(key == KEY_RIGHT)
					selected -= items.rows;
				else if(key == KEY_UP)
					selected--;
				else if(key == KEY_DOWN)
					selected++;
				else
					return;
			}
		}
		else {//XIM_STATE_RELEASE
			if(key == KEY_ENTER || key == KEY_BUTTON_START || key == KEY_BUTTON_A) {
				runProc(&items.items[selected]);
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

	void onMouse(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);

		int itemW = xinfo.wsr.w / items.cols;
		int itemH = xinfo.wsr.h / items.rows;
		int col = (ev->value.mouse.x - xinfo.wsr.x) / itemW;
		int row = (ev->value.mouse.y - xinfo.wsr.y) / itemH;
		int at;
		if(position == POS_TOP || position == POS_BOTTOM) 
			at = row*items.cols + col + start;
		else if(position == POS_LEFT) 
			at = col*items.rows + row + start;
		else
			at = (items.cols-col-1)*items.rows + row + start;
		if(at >= items.num)
			return;

		if(ev->state == XEVT_MOUSE_DOWN) {
			if(selected != at) {
				selected = at;
				repaint();
			}
		}
		else if(ev->state == XEVT_MOUSE_CLICK) {
			runProc(&items.items[at]);
			return;
		}
	}

	void onEvent(xevent_t* ev) {
		if(ev->type == XEVT_MOUSE) {
			onMouse(ev);
		}
		else if(ev->type == XEVT_IM) {
			onIM(ev);
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

	void checkProc(void) {
		bool doRepaint = false;
		for(int i=0; i<items.num; i++) {
			item_t* item = &items.items[i];

			if(item->runPid > 0)  {
				if(proc_check_uuid(item->runPid, item->runPidUUID) != item->runPidUUID) {
					item->runPid = 0;
					doRepaint = true;
				}
			}
		}
		if(doRepaint) {
			repaint();
		}
	}

	bool readConfig(const char* fname) {
		items.marginH = 6;
		items.marginV = 2;
		items.icon_size = 64;
		titleColor = 0xffffffff;
		bgColor = 0xff000000;
		selectedColor = 0x88444444;
		height = 0;
		width = 0;
		fontSize = 14;
		position = POS_BOTTOM;

		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		const char* v = sconf_get(conf, "icon_size");
		if(v[0] != 0)
			items.icon_size = atoi(v);

		v = sconf_get(conf, "position");
		if(v[0] == 'b')
			position = POS_BOTTOM;
		else if(v[0] == 't')
			position = POS_TOP;
		else if(v[0] == 'l')
			position = POS_LEFT;
		else if(v[0] == 'r')
			position = POS_RIGHT;

		v = sconf_get(conf, "marginH");
		if(v[0] != 0)
			items.marginH = atoi(v);

		v = sconf_get(conf, "marginV");
		if(v[0] != 0)
			items.marginV = atoi(v);

		v = sconf_get(conf, "font_size");
		if(v[0] != 0)
			fontSize = atoi(v);

		v = sconf_get(conf, "font");
		if(v[0] == 0)
			v = DEFAULT_SYSTEM_FONT;
		font_load(v, fontSize, &font, true);

		v = sconf_get(conf, "title_color");
		if(v[0] != 0)
			titleColor = atoi_base(v, 16);

		v = sconf_get(conf, "bg_color");
		if(v[0] != 0)
			bgColor = atoi_base(v, 16);

		v = sconf_get(conf, "icon_selected_color");
		if(v[0] != 0)
			selectedColor = atoi_base(v, 16);

		sconf_free(conf);
		return true;
	}

	inline uint32_t getHeight(const xscreen_t& scr) {
		if((position == POS_LEFT || position == POS_RIGHT) && items.cols > 1)
			height = scr.size.h;
		else
			height = (fontSize + items.icon_size + titleMargin + items.marginV) *
					items.rows + items.marginV;
		return height;
	}

	inline uint32_t getWidth(const xscreen_t& scr) {
		if((position == POS_TOP || position == POS_BOTTOM) && items.rows > 1)
			width = scr.size.w;
		else 
			width = (items.icon_size + items.marginH) *
					items.cols + items.marginH;
		return width;
	}

	inline gpos_t getPos(const xscreen_t& scr) {
		gpos_t pos;
		if(position == POS_BOTTOM) {
			pos.x = (scr.size.w-width)/2;
			pos.y = scr.size.h - height;
		}
		else if(position == POS_TOP) {
			//pos.x = (scr.size.w-width)/2;
			pos.x = 0;
			pos.y = 0;
		}
		else if(position == POS_LEFT) {
			pos.x = 0;
			pos.y = 0;
			pos.y = (scr.size.h-height)/2;
		}
		else if(position == POS_RIGHT) {
			pos.x = scr.size.w - width;
			pos.y = 0;
			//pos.y = (scr.size.h-height)/2;
		}
		return pos;
	}

	str_t* getIconFname(const char* appName) {
		//try theme icon first
		const char* theme = x_get_theme();
		str_t* ret = NULL;
		if(theme[0] != 0) {
			ret = str_new(x_get_theme_fname(X_THEME_ROOT, appName, "icon.png"));
			if(vfs_access(ret->cstr) == 0)
				return ret;
			str_free(ret);
		}

		ret = str_new("/apps/");
		str_add(ret, appName);
		str_add(ret, "/res/icon.png");
		return ret;
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

			items.items[i].icon = getIconFname(it->d_name);
			i++;
		}
		items.num = i;
		closedir(dirp);
		return true;
	}

	void layout(const xscreen_t& scr) {
		int max = 0;
		if(position == POS_TOP || position == POS_BOTTOM) {
			int max = (scr.size.w - items.marginH) / (items.icon_size + items.marginH);
			if(items.num > max)
				items.cols = max;
			else
				items.cols = items.num;
			if(items.cols > 0)
				items.rows = items.num / items.cols;
			if((items.cols*items.rows) != items.num)
				items.rows++;
			return;
		}

		max = (scr.size.h - items.marginV) /
				(fontSize + items.icon_size + titleMargin + items.marginV);
		if(items.num > max)
			items.rows = max;
		else
			items.rows = items.num;
		if(items.rows > 0)
			items.cols = items.num / items.rows;
		if((items.cols*items.rows) != items.num)
			items.cols++;
	}
};

static void check_proc(void* p) {
	Launcher* xwin = (Launcher*)p;
	xwin->checkProc();
	usleep(20000);
}

int main(int argc, char* argv[]) {
	(void)argc;
	(void)argv;

	X x;
	xscreen_t scr;
	x.screenInfo(scr, 0);

	Launcher xwin;
	const char* cfg = x_get_theme_fname(X_THEME_ROOT, "launcher", "theme.conf");
	xwin.readConfig(cfg);
	xwin.loadApps();
	xwin.layout(scr);

	int32_t h = xwin.getHeight(scr);
	int32_t w = xwin.getWidth(scr);
	gpos_t pos = xwin.getPos(scr);

	x.open(&xwin, pos.x, pos.y, w, h, "launcher",
			X_STYLE_NO_FRAME | X_STYLE_LAUNCHER | X_STYLE_SYSBOTTOM);

	xwin.setVisible(true);

	x.run(check_proc, &xwin);
	return 0;
}
