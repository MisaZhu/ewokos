#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/List.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/proc.h>
#include <tinyjson/tinyjson.h>
#include <upng/upng.h>
#include <dirent.h>

#include <string>
using namespace EwokSTL;
using namespace Ewok;

typedef struct {
	string app;
	string fname;
	string icon;
	graph_t* iconImg;
} item_t;

#define ITEM_MAX 128

class AppList: public List {
	item_t items[ITEM_MAX];	
	static const uint32_t iconSize = 32;
	static const uint32_t titleMargin = 4;

	string getIconFname(const char* appName) {
		//try theme icon first
		string ret = x_get_theme_fname(X_THEME_ROOT, appName, "icon.png");
		if(access(ret.c_str(), R_OK) == 0)
			return ret;

		ret = "/apps/";
		ret = ret + appName + "/res/icon.png";
		return ret;
	}
 
	void drawIcon(graph_t* g, int at, const Theme* theme, int x, int y, int w, int h) {
		item_t* item = &items[at];
		const char* icon = item->icon.c_str();
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
		int dy = (h - (int)(iconSize + titleMargin + theme->font->max_size.y)) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, const Theme* theme, int x, int y, int w, int h) {
		const char* title = items[at].app.c_str();
		uint32_t tw, th;
		font_text_size(title, theme->font, &tw, &th);
		x += (w - (int32_t)tw)/2;
		y += (h - (int)(iconSize + titleMargin + (int32_t)th)) /2 +
				iconSize + titleMargin;
		graph_draw_text_font(g, x, y, title, theme->font, theme->fgColor);
	}

protected:
	void drawBG(graph_t* g, const Theme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, 0xffbbbbbb);
	}

	void drawItem(graph_t* g, const Theme* theme, int32_t index, const grect_t& r) {
		if(index >= itemNum)
			return;

		drawIcon(g, index, theme, r.x , r.y, r.w, r.h);
		drawTitle(g, index, theme, r.x , r.y, r.w, r.h);
	}

	void onEnter(int32_t index) {
		int pid = fork();
		if(pid == 0) {
			proc_exec(items[index].fname.c_str()); 
		}
	}

public:
	AppList() {
		for(int i=0; i<ITEM_MAX; i++)
			memset(&items[i], 0, sizeof(item_t));
	}

	bool loadApps(void) {
		int sz = 0;
		var_t* var = NULL;
		char* str = (char*)vfs_readfile("/usr/system/xlauncher.json", &sz);
		if(str == NULL) {
			return false;
		}
		str[sz] = 0;
		var = json_parse(str);
		
		var_t* apps = get_obj(var, "apps");
		if(apps == NULL) {
			var_unref(var);
			return false;
		}
		
		sz = var_array_size(apps);
		for(int i=0;i <sz && i <ITEM_MAX; i++) {
			var_t* app = var_array_get_var(apps, i);
			if(app == NULL)
				continue;

			const char* name = get_str(app, "name");
			const char* file = get_str(app, "file");

			items[i].app = name;
			items[i].fname = file;
			items[i].icon = file;
			items[i].icon = getIconFname(name);
		}

		setItemNum(sz);
		return true;
	}

	uint32_t getNum() {
		return itemNum;
	}
};

int main(int argc, char** argv) {
	X x;
	WidgetWin win;
	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	root->setAlpha(false);

	AppList* apps = new AppList();
	apps->loadApps();
	apps->setHorizontal(true);
	root->add(apps);


	grect_t desk;
	x.getDesktopSpace(desk, 0);

	uint32_t itemSize = 64;
	uint32_t itemNum = apps->getNum();
	apps->setItemSize(itemSize);

	x.open(0, &win, (desk.w-itemSize*itemNum)/2, desk.h-itemSize,
			itemSize*itemNum, itemSize, "xlauncher",
			XWIN_STYLE_NO_TITLE | XWIN_STYLE_LAUNCHER | XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_ANTI_FSCR);

	win.setVisible(true);
	x.run(NULL, &win);
	return 0;
}