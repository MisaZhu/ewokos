#include <Widget/WidgetWin.h>
#include <Widget/WidgetX.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/Grid.h>
#include <Widget/Scroller.h>
#include <x++/X.h>
#include <unistd.h>
#include <ewoksys/basic_math.h>
#include <ewoksys/kernel_tic.h>
#include <ewoksys/proc.h>
#include <graph/graph_png.h>
#include <dirent.h>

#include <getopt.h>

#ifdef __cplusplus
extern "C" { extern int setenv(const char*, const char*);}
#endif

#include <string>
using namespace std;
using namespace Ewok;

typedef struct {
	string app;
	string fname;
	string icon;
	graph_t* iconImg;
} item_t;

#define ITEM_MAX 128

class AppGrid: public Grid {
	item_t items[ITEM_MAX];	
	static const uint32_t titleMargin = 8;
	uint32_t iconSize;

	string getIconFname(const char* appName) {
		//try theme icon first
		string ret = x_get_theme_fname(X_THEME_ROOT, appName, "icon.png");
		if(access(ret.c_str(), R_OK) == 0)
			return ret;

		ret = "/apps/";
		ret = ret + appName + "/res/icon.png";
		return ret;
	}

	void clearItem() {
		for(int i=0; i<ITEM_MAX; i++) {
			if(items[i].iconImg != NULL) {
				graph_free(items[i].iconImg);
				items[i].iconImg = NULL;
			}
		}
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
		int dy = (h - (int)(iconSize + titleMargin + theme->basic.fontSize)) / 2;
		graph_blt_alpha(img, 0, 0, img->w, img->h,
				g, x+dx, y+dy, img->w, img->h, 0xff);
	}

	void drawTitle(graph_t* g, int at, XTheme* theme, int x, int y, int w, int h) {
		const char* title = items[at].app.c_str();
		uint32_t tw, th;
		font_text_size(title, theme->getFont(), theme->basic.fontSize, &tw, &th);
		x += (w - (int32_t)tw)/2;
		y += (h - (int)(iconSize + titleMargin + (int32_t)th)) /2 +
				iconSize + titleMargin;
		graph_draw_text_font(g, x, y, title, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor);
	}

protected:
	void drawBG(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	}

	void drawItem(graph_t* g, XTheme* theme, int32_t index, const grect_t& r) {
		if(index >= itemNum)
			return;

		if(index == itemSelected)
			graph_fill_round(g, r.x, r.y, r.w, r.h, 6, theme->basic.selectBGColor);

		drawIcon(g, index, theme, r.x , r.y, r.w, r.h);
		drawTitle(g, index, theme, r.x , r.y, r.w, r.h);
	}

	void onEnter(int index) {
		x_exec(items[index].fname.c_str());
	}
public:
	AppGrid() {
		iconSize = 32;
		scrollerV = NULL;
		for(int i=0; i<ITEM_MAX; i++)
			memset(&items[i], 0, sizeof(item_t));
	}

	~AppGrid() {
		clearItem();
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
			items[i].app = it->d_name;
			items[i].fname = "/apps/";
			items[i].fname = items[i].fname + it->d_name + "/" +  it->d_name;
			items[i].icon = getIconFname(it->d_name);
			i++;
		}
		setItemNum(i);
		closedir(dirp);
		return true;
	}

	void setIconSize(uint32_t size) {
		iconSize = size;
	}
};

static bool _launcher = false;
static uint32_t _itemSize = 72;
static uint32_t _fontSize = 0;

static int doargs(int argc, char* argv[]) {
	int opt_index = 0;
    static struct option long_opts[] = {
        {"launcher", no_argument, 0, 'l'},
        {"item_size",  required_argument, 0, 'i'},
        {"font_size",  required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };

	int c = 0;
	while (c != -1) {
		c = getopt_long (argc, argv, "li:f:", long_opts, &opt_index);
		if(c == -1)
			break;

		switch (c) {
		case 'l':
			_launcher = true;
			break;
		case 'i':
			_itemSize = atoi(optarg);
			break;
		case 'f':
			_fontSize = atoi(optarg);
			break;
		default:
			c = -1;
			break;
		}
	}
	return optind;
}

int main(int argc, char** argv) {
	_launcher = false;
	doargs(argc, argv);

	X x;
	WidgetWin win;
	if(_fontSize > 0)
		win.getTheme()->setFont("system", _fontSize);

	RootWidget* root = new RootWidget();
	win.setRoot(root);
	root->setType(Container::HORIZONTAL);
	

	AppGrid* apps = new AppGrid();
	apps->setItemSize(_itemSize, _itemSize);
	apps->setIconSize(_itemSize/2);
	root->add(apps);
	root->focus(apps);

	Scroller* scrollerV = new Scroller();
	scrollerV->fix(8, 0);
	root->add(scrollerV);
	apps->setScrollerV(scrollerV);

	if(_launcher) {
		win.open(&x, 0, 0, 0, 320, 240, "xapps", 
			XWIN_STYLE_NO_TITLE | XWIN_STYLE_LAUNCHER | XWIN_STYLE_NO_BG_EFFECT);
			//XWIN_STYLE_NO_TITLE | XWIN_STYLE_LAUNCHER | XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_NO_BG_EFFECT);
		win.max();
	}
	else
		win.open(&x, 0, -1, -1, 320, 240, "xapps", XWIN_STYLE_NORMAL);

	win.busy(true);
	apps->loadApps();
	win.repaint();
	win.busy(false);

	widgetXRun(&x, &win);
	return 0;
}