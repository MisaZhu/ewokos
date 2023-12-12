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
	int  powerFD;

	void drawCharging(graph_t* g, const grect_t& r, int bat) {
		static int b = 0;
		int w = r.w*bat*b/300;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, 0xff22dd22, true);
		b++;
		b%=4;
	}

	void drawBat(graph_t* g, const grect_t& r, int bat) {
		int w = r.w*bat/100;
		uint32_t color = (bat <= 20) ?0xffed7930 : 0xff22dd22;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, color, true);
	}

	void drawBase(graph_t* g, grect_t& r) {
		graph_gradation(g, r.x, r.y+2, 3, r.h-4, 0xffffffff, 0xffaaaaaa, true);
		graph_box(g, r.x, r.y+2, 3, r.h-4, 0xff000000);
		r.x += 2;
		r.w -= 2;

		graph_gradation(g, r.x, r.y, r.w, r.h, 0xffffffff, 0xff888888, true);
		graph_box(g, r.x, r.y, r.w, r.h, 0xff000000);
		r.x++;
		r.y++;
		r.w -= 2;
		r.h -= 2;
	}

protected:
	void onRepaint(graph_t* g, const Theme* theme, const grect_t& rect) {
		setAlpha(true);
		grect_t r = rect;
		drawBase(g, r);

		if(powerFD < 0)
			powerFD = open("/dev/power0", O_RDONLY);

		if(powerFD < 0)
			return;

		uint8_t buf[4];
		if(read(powerFD, buf, 3) != 3)
			return;
			
		if(buf[0] == 0)
			return;

		if(buf[1])
			drawCharging(g, r, buf[2]);
		else
			drawBat(g, r, buf[2]);
	}

	gsize_t getMinSize() {
		return {56, 28};
	}
public:
	BatteryItem() {
		powerFD = -1;
		setMarginV(6);
		setMarginH(4);
	}

	~BatteryItem() {
		if(powerFD > 0)
			::close(powerFD);
	}
};

X* _x = NULL;
class MenubarItem : public Label {
	Menu *menu;
protected:
	void onClick() {
		grect_t r = getScreenArea();

		if(menu == NULL) {
			menu = new Menu();
			_x->open(menu, r.x, r.y+r.h, 100, 100, "menu", XWIN_STYLE_NO_FRAME);
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

		Widget* wd = new Image("/usr/system/icons/os.png");
		wd->setMarginH(10);
		wd->fix(wd->getMinSize());
		container->add(wd);
	
		wd = new MenubarItem(X::getSysFont(), "EwokOS");
		wd->setMarginH(10);
		wd->fix(wd->getMinSize());
		container->add(wd);

		wd = new MenubarItem(X::getSysFont(), "EwokOS1");
		wd->setMarginH(10);
		wd->fix(wd->getMinSize());
		container->add(wd);

		wd = new Widget();
		container->add(wd);

		wd = new BatteryItem();
		wd->setMarginH(10);
		wd->fix(wd->getMinSize());
		container->add(wd);
	}

	~Menubar() {
	}
};

static Menubar* _xwin = NULL;
static void timer_handler(void) {
	_xwin->getRootWidget()->update();
	_xwin->getRootWidget()->repaintWin();
}

int main(int argc, char* argv[]) {
	X x;
	_x = &x;
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

