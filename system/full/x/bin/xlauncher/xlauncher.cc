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
	uint32_t iconSize;
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
 
	void drawIcon(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
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
		int dy = (h - (int)(iconSize + titleMargin + theme->getFont()->max_size.y)) / 2;
		if(at == itemSelected)
			dy -= 4;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
		const char* title = items[at].app.c_str();
		uint32_t tw, th;
		font_text_size(title, theme->getFont(), &tw, &th);
		x += (w - (int32_t)tw)/2;
		y += (h - (int)(iconSize + titleMargin + (int32_t)th)) /2 +
				iconSize + titleMargin;

		graph_draw_text_font(g, x, y, title, theme->getFont(), 0xff000000);
		if(at == itemSelected)
			graph_draw_text_font(g, x-1, y-1, title, theme->getFont(), 0xffffffff);
	}

	bool loadApps(var_t* var) {
		if(var == NULL) {
			return false;
		}

		var_t* apps = get_obj(var, "apps");
		if(apps == NULL) {
			return false;
		}
		
		int sz = var_array_size(apps);
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

protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, 0xffbbbbbb);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
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
		iconSize = 32;
		for(int i=0; i<ITEM_MAX; i++)
			memset(&items[i], 0, sizeof(item_t));
	}

	bool loadConfig() {
		int sz = 0;
		var_t* var = NULL;
		char* str = (char*)vfs_readfile("/usr/x/xlauncher/xlauncher.json", &sz);
		if(str == NULL) {
			return false;
		}
		str[sz] = 0;
		var = json_parse(str);
		free(str);
		if(var == NULL)
			return false;

		loadApps(var);

		itemSize = get_int(var, "item_size");
		if(itemSize == 0)
			itemSize = 64;

		iconSize = get_int(var, "icon_size");
		if(iconSize == 0)
			iconSize = 32;
		
		var_unref(var);
		return true;
	}

	void setIconSize(uint32_t size) {
		iconSize = size;
		onResize();
		update();
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
	apps->loadConfig();
	root->add(apps);

	grect_t desk;
	x.getDesktopSpace(desk, 0);

	uint32_t itemSize = apps->getItemSize();
	uint32_t itemNum = apps->getItemNum();

	grect_t wr;
	if(argc >=2 && strcmp(argv[1], "left") == 0) {
		wr.x = 0;
		wr.y = (desk.h-itemSize*itemNum)/2,
		wr.w = itemSize;
		wr.h = itemSize*itemNum;
	}
	else if(argc >=2 && strcmp(argv[1], "right") == 0) {
		wr.x = desk.w - itemSize;
		wr.y = (desk.h-itemSize*itemNum)/2,
		wr.w = itemSize;
		wr.h = itemSize*itemNum;
	}
	else {
		wr.x = (desk.w-itemSize*itemNum)/2;
		wr.y = desk.h-itemSize;
		wr.w = itemSize*itemNum;
		wr.h = itemSize;
		apps->setHorizontal(true);
	}

	x.open(0, &win, wr, "xlauncher",
				XWIN_STYLE_NO_TITLE | XWIN_STYLE_LAUNCHER | XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_ANTI_FSCR);
	win.setVisible(true);
	x.run(NULL, &win);
	return 0;
}