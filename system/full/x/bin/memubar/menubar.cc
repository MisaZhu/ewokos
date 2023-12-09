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
		Container* container = getRootWidget();
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

class BatteryItem : public Widget {
	int  power;

	void drawNoBat(graph_t* g, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, 0xff222222);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
	}

	void drawCharging(graph_t* g, const grect_t& r, int bat) {
		static bool b = true;
		int w = r.w*bat/100;
		if(b)
			graph_fill(g, r.x+r.w-w, r.y, w, r.h, 0xffdddddd);
		else
			graph_fill(g, r.x+r.w-w, r.y, w, r.h, 0xff22dd22);

		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
		b = !b;
	}

	void drawBat(graph_t* g, const grect_t& r, int bat) {
		int w = r.w*bat/100;
		graph_fill(g, r.x+r.w-w, r.y, w, r.h, 0xff22dd22);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
	}

protected:
	void onRepaint(graph_t* g) {
		grect_t r = getRootArea(true);
		graph_box(g, r.x, r.y+2, 5, r.h-4, 0xff000000);
		r.x += 4;
		r.w -= 4;

		uint8_t buf[4];
		if(power < 0){
			drawNoBat(g, r);
			return;
		}

		if(read(power, buf, 3) == 3){
			if(!buf[0])
				drawNoBat(g, r);
			else if(buf[1])
				drawCharging(g, r, buf[2]);
			else
				drawBat(g, r, buf[2]);
		}
		else {
			drawNoBat(g, r);
		}
	}

	gsize_t getMinSize() {
		return {56, 28};
	}
public:
	BatteryItem() {
		power = open("/dev/power0", O_RDONLY);
		setMarginV(6);
		setMarginH(4);
	}
};

class MenubarItem : public Label {
	Menu *menu;
protected:
	void onClick() {
		grect_t r = getScreenArea();

		if(menu == NULL) {
			X *x = getRoot()->getWin()->getX();
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
		RootWidget* container = getRootWidget();
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

		wd = new Widget();
		container->add(wd);

		wd = new BatteryItem();
		wd->setMarginH(10);
		wd->fixedMinSize();
		container->add(wd);
	}

	~Menubar() {
	}
};

static Menubar* _xwin = NULL;
static void timer_handler(void) {
	_xwin->getRootWidget()->update();
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

