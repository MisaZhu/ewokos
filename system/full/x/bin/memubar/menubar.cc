#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/keydef.h>
#include <upng/upng.h>
#include <font/font.h>
#include <x++/X.h>
#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <fcntl.h>
#include <string.h>
#include <sys/timer.h>

using namespace Ewok;

class Menu : public WidgetWin {
protected:
	void onRepaint(graph_t* g) {
		setAlpha(true);
		graph_fill(g, 2, 2, g->w-2, g->h-2, 0xff000000);
		graph_fill(g, 0, 0, g->w-2, g->h-2, 0xffffffff);
		graph_box(g, 0, 0, g->w-2, g->h-2, 0xff000000);
		WidgetWin::onRepaint(g);
	}

	void onUnfocus() {
		setVisible(false);
	}
public:
	Menu() {
		Container* container = getWidget();
		container->setType(Container::VERTICLE);
	
		Widget* wd = new Label(X::getSysFont(), "item1");
		wd->setMarginV(4);
		//wd->fixedMinSize();
		container->add(wd);

		wd = new Label(X::getSysFont(), "item2");
		wd->setMarginV(4);
		//wd->fixedMinSize();
		container->add(wd);

		wd = new Label(X::getSysFont(), "item3");
		wd->setMarginV(4);
		//wd->fixedMinSize();
		container->add(wd);
	}
};

class BatteryItem : public Label {
private: 
	char text[32] = {0};
	int  power;
protected:
	void onRepaint(graph_t* g, grect_t* rect) {
		uint8_t buf[4];
		if(power > 0){
			if(read(power, buf, 3) == 3){
				if(!buf[0]){
					snprintf(text, sizeof(text), "NO");
				}else if(buf[1]){
					snprintf(text, sizeof(text), "CH");
				}else{
					snprintf(text, sizeof(text), "%d", buf[2]);
				}
				label = (const char*)text;
			}
		}
		Label::onRepaint(g, rect);
	}
public:
	BatteryItem(): Label(X::getSysFont(), text) { 
		power = open("/dev/power0", O_RDONLY);
	}
};

class MenubarItem : public Label {
	Menu *menu;
protected:
	void onClick() {
		grect_t r = getAbsRect();

		if(menu == NULL) {
			X *x = getWin()->getX();
			menu = new Menu();
			x->open(menu, r.x, r.y+r.h, 100, 100, "menu", XWIN_STYLE_NO_FRAME);
		}
		menu->pop();
	}

	bool onMouse(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_CLICK) {
			onClick();
		}
		return true;
	}
public:
	MenubarItem(font_t* font, const char* str): Label(font, str) { 
		menu = NULL;
	}

	
};

class Menubar : public WidgetWin {
public:
	Menubar() {
		Container* container = getWidget();
		container->setType(Container::HORIZONTAL);
		container->setBGColor(0xffffffff);

		Widget* wd = new Image("/usr/system/icons/os.png");
		wd->setMarginH(10);
		wd->fixedMinSize();
		container->add(wd);
	
		wd = new MenubarItem(X::getSysFont(), "EwokOS");
		wd->setMarginH(10);
		wd->fixedMinSize();
		container->add(wd);

		wd = new BatteryItem();
		wd->setMarginH(10);
		wd->fixedMinSize();
		container->add(wd);


		wd = new Widget();
		container->add(wd);
	}

	~Menubar() {
	}
};

static XWin* _xwin = NULL;
static void timer_handler(void) {
	_xwin->repaint();
}

int main(int argc, char* argv[]) {
	X x;
	xscreen_t scr;
	x.getScreenInfo(scr, 0);
	Menubar xwin;
	x.open(&xwin, 0, 0, scr.size.w, 28, "menubar",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_FOCUS| XWIN_STYLE_SYSBOTTOM);
	xwin.setVisible(true);

	grect_t desk;
	x.getDesktopSpace(desk, 0);
	desk.y += 28;
	desk.h -= 28;
	x.setDesktopSpace(desk);

	_xwin = &xwin;
	uint32_t tid = timer_set(1000000, timer_handler);

	x.run(NULL);
	timer_remove(tid);
	return 0;
}

