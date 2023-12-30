#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sconf/sconf.h>
#include <upng/upng.h>
#include <x++/X.h>
#include <ewoksys/keydef.h>
#include <ewoksys/klog.h>
#include <ewoksys/proc.h>
#include <font/font.h>
#include <dirent.h>
#include <Widget/ListView.h>

#define ITEM_MAX 16
using namespace Ewok;

typedef struct {
	str_t* app;
	str_t* fname;
	str_t* icon;
	graph_t* iconImg;
	int runPid;
	uint32_t runPidUUID;
} item_t;

class LauncherView: public ListView {
	item_t items[ITEM_MAX];	
	uint32_t itemNum;
	font_t* font;
	uint32_t titleColor;
	uint32_t bgColor;
	uint32_t selectedColor;
	uint32_t fontSize;
	uint32_t iconSize;
	int32_t titleMargin;

	void drawRunning(graph_t* g, int x, int y) {
		uint32_t mV = itemsInfo.marginV/2;
		uint32_t mH = itemsInfo.marginH/2;
		uint32_t r = mV>mH ? mH:mV;
		graph_fill_circle(g, x+2, y+2, r, 0x88000000);
		graph_fill_circle(g, x, y, r-1, 0xff000000);
		graph_circle(g, x, y, r, 0xffffffff);
	}

	void drawIcon(graph_t* g, int at, int x, int y, int w, int h) {
		item_t* item = &items[at];
		const char* icon = item->icon->cstr;
		int icon_size = iconSize < w ? iconSize : w;
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
		int dy = (h - (int)(iconSize + titleMargin + fontSize)) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
		
		if(item->runPid > 0)
			drawRunning(g, x+dx, y+dy);
	}

	void drawTitle(graph_t* g, int at, int x, int y, int w, int h) {
		const char* title = items[at].app->cstr;
		uint32_t tw, th;
		font_text_size(title, font, &tw, &th);
		x += (w - (int32_t)tw)/2;
		y += (h - (int)(iconSize + titleMargin + (int32_t)th)) /2 +
				iconSize + titleMargin;
		graph_draw_text_font(g, x, y, title, font, titleColor);
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
	void onClick(uint32_t at) {
		item_t* item = &items[at];
		runProc(item);
	}

	void drawItem(graph_t* g, int at, int x, int y, int w, int h) {
		if (selected == at) {
			graph_fill_round(g, x, y,
							w, h,
							8, selectedColor);
		}
		drawIcon(g, at, x , y, w, h);
		drawTitle(g, at, x, y, w, h);
	}

	uint32_t getItemNum() {
		return itemNum;
	}

	void drawBG(graph_t* g, const gpos_t& pos) {
		graph_clear(g, bgColor);
	}

public:
	inline LauncherView() {
		font = NULL;
		itemNum = 0;
		titleMargin = 2;
		for(int i=0; i<ITEM_MAX; i++)
			memset(&items[i], 0, sizeof(item_t));
	}

	inline ~LauncherView() {
		for(int i=0; i<itemNum; i++) {
			str_free(items[i].app);
			str_free(items[i].fname);
			str_free(items[i].icon);
			if(items[i].iconImg)
				graph_free(items[i].iconImg);
		}
		if(font != NULL)
			font_free(font);
	}

	bool readConfig(const char* fname) {
		itemsInfo.marginH = 6;
		itemsInfo.marginV = 2;
		iconSize = 64;
		titleColor = 0xffffffff;
		bgColor = 0xff000000;
		selectedColor = 0x88444444;
		fontSize = 14;
		position = POS_BOTTOM;

		sconf_t *conf = sconf_load(fname);	
		if(conf == NULL)
			return false;
		const char* v = sconf_get(conf, "icon_size");
		if(v[0] != 0)
			iconSize = atoi(v);

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
			itemsInfo.marginH = atoi(v);

		v = sconf_get(conf, "marginV");
		if(v[0] != 0)
			itemsInfo.marginV = atoi(v);

		v = sconf_get(conf, "font_size");
		if(v[0] != 0)
			fontSize = atoi(v);

		v = sconf_get(conf, "font");
		if(v[0] == 0)
			v = DEFAULT_SYSTEM_FONT;
		font = font_new(v, fontSize, true);

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

		itemsInfo.itemSize.h = fontSize + iconSize + titleMargin;
		itemsInfo.itemSize.w = iconSize;
		return true;
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
			items[i].app = str_new(it->d_name);

			items[i].fname = str_new("/apps/");
			str_add(items[i].fname, it->d_name);
			str_add(items[i].fname, "/");
			str_add(items[i].fname, it->d_name);

			items[i].icon = getIconFname(it->d_name);
			i++;
		}
		itemNum = i;
		closedir(dirp);
		return true;
	}

	bool checkProc(void) {
		bool doRepaint = false;
		for(int i=0; i<itemNum; i++) {
			item_t* item = &items[i];
			if(item->runPid > 0)  {
				if(proc_check_uuid(item->runPid, item->runPidUUID) != item->runPidUUID) {
					item->runPid = 0;
					doRepaint = true;
				}
			}
		}
		return doRepaint;
	}
};

class Launcher: public XWin {
	LauncherView launcherView;
protected:
	void onRepaint(graph_t* g) {
		setAlpha(true);
		launcherView.repaint(g);
	}

	void onEvent(xevent_t* ev) {
		xinfo_t xinfo;
		getInfo(xinfo);
		launcherView.handleEvent(ev, xinfo.wsr);
		repaint();
	}

	void onResize(void) {
		xinfo_t xinfo;
		getInfo(xinfo);
		launcherView.layout(xinfo.wsr);
	}

public:
	Launcher() {
	}

	~Launcher() {
	}

	inline LauncherView* getView() {
		return &launcherView;
	}

	void checkProc(void) {
		if(launcherView.checkProc()) {
			repaint();
		}
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
	grect_t desk;
	x.getDesktopSpace(desk, 0);

	Launcher xwin;
	LauncherView* view = xwin.getView();
	const char* cfg = x_get_theme_fname(X_THEME_ROOT, "launcher", "theme.conf");

	view->readConfig(cfg);
	view->loadApps();
	view->layout(desk);

	gsize_t sz = view->getSize();
	gpos_t pos = view->getPos(desk);

	x.open(0, &xwin, pos.x, pos.y, sz.w, sz.h, "launcher",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_LAUNCHER | XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_ANTI_FSCR);
	xwin.setVisible(true);

	x.run(check_proc, &xwin);
	return 0;
}
