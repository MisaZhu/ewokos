#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ewoksys/keydef.h>
#include <upng/upng.h>
#include <font/font.h>
#include <x++/X.h>
#include <Widget/WidgetWin.h>
#include <Widget/Image.h>
#include <Widget/Label.h>
#include <Widget/LabelButton.h>
#include <Widget/Blank.h>
#include <fcntl.h>
#include <string.h>
#include <ewoksys/timer.h>
#include <ewoksys/kernel_tic.h>

using namespace Ewok;

class Menu : public RootWidget {
public:
	Menu() {
		setType(Container::VERTICLE);
	
		Widget* wd = new Label("item1");
		wd->setMarginH(4);
		wd->setMarginV(4);
		add(wd);

		wd = new Label("item2");
		wd->setMarginH(4);
		wd->setMarginV(4);
		add(wd);

		wd = new Label("item3");
		wd->setMarginH(4);
		wd->setMarginV(4);
		add(wd);
	}
};
class MenuWin : public WidgetWin {

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
	MenuWin() {
		Menu *menu = new Menu();
		setRoot(menu);
	}
};

class Timer: public Label {
protected:
	void onTimer() {
		uint32_t ksec;
		kernel_tic(&ksec, NULL);
		uint32_t min = ksec / 60;
		uint32_t hour = min / 60;
		min = min % 60;
		ksec = ksec % 60;
		char s[16];
		snprintf(s, 15, "%02d:%02d:%02d", hour, min, ksec);
		setLabel(s);
	}
public: 
	Timer(const string& label = "00:00:00") : Label(label) {
	}
};

class BatteryItem : public Widget {
	int  powerFD;

	void drawCharging(graph_t* g, XTheme* theme, const grect_t& r, int bat) {
		static int b = 0;
		int w = r.w*bat*b/300;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, 0xff22dd22, true);

		char txt[8];
		snprintf(txt, 7, "%d%%", bat);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
		b++;
		b%=4;
	}

	void drawBat(graph_t* g, XTheme* theme, const grect_t& r, int bat) {
		int w = r.w*bat/100;
		uint32_t color = (bat <= 20) ?0xffed7930 : 0xff22dd22;
		graph_gradation(g, r.x+r.w-w, r.y, w, r.h, 0xffffffff, color, true);

		char txt[8];
		snprintf(txt, 7, "%d%%", bat);
		graph_draw_text_font_align(g, r.x, r.y, r.w, r.h,
					txt, theme->getFont(), theme->basic.fontSize, theme->basic.fgColor, FONT_ALIGN_CENTER);
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
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& rect) {
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
			drawCharging(g, theme, r, buf[2]);
		else
			drawBat(g, theme, r, buf[2]);
	}

	gsize_t getMinSize() {
		return {56, 28};
	}
	
	void onTimer() {
		update();
	}

public:
	BatteryItem() {
		powerFD = -1;
		setMarginV(4);
		setMarginH(4);
	}

	~BatteryItem() {
		if(powerFD > 0)
			::close(powerFD);
	}
};

X* _x = NULL;
class MenubarItem : public Label {
	MenuWin *menu;
protected:
	void onClick() {
		grect_t r = getScreenArea();

		if(menu == NULL) {
			menu = new MenuWin();
			menu->open(_x, 0, r.x, r.y+r.h, 100, 100, "menu", XWIN_STYLE_NO_FRAME);
		}
		menu->pop();
	}

	bool onMouse(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_CLICK)
			onClick();
		return true;
	}

public:
	MenubarItem(const string& str): Label(str) { 
		menu = NULL;
	}

	~MenubarItem() {
		if(menu != NULL) {
			menu->close();
			delete menu;
		}
	}
};

class Menubar : public RootWidget {
protected:
	void onRepaint(graph_t* g, XTheme* theme, const grect_t& r) {
		graph_fill(g, r.x, r.y, r.w, r.h, theme->basic.bgColor);
	}

public:
	Menubar() {
		setType(Container::HORIZONTAL);

		Widget* wd = new Image("/usr/system/icons/os.png");
		wd->setMarginH(10);
		wd->fix(wd->getMinSize());
		add(wd);
	
		wd = new MenubarItem("EwokOS");
		wd->setMarginH(4);
		wd->fix(64, 0);
		add(wd);

		wd = new MenubarItem("EwokOS1");
		wd->setMarginH(4);
		wd->fix(64, 0);
		add(wd);

		wd = new Blank();
		add(wd);

		wd = new Timer();
		wd->setMarginH(4);
		wd->fix(64, 0);
		add(wd);

		wd = new BatteryItem();
		wd->setMarginH(4);
		wd->fix(wd->getMinSize());
		add(wd);
	}
};

int main(int argc, char* argv[]) {
	X x;
	_x = &x;
	xscreen_t scr;
	x.getScreenInfo(scr, 0);

	WidgetWin xwin;
	xwin.setRoot(new Menubar());

	xwin.open(&x, 0, 0, 0, scr.size.w, 28, "menubar",
			XWIN_STYLE_NO_FRAME | XWIN_STYLE_NO_FOCUS| XWIN_STYLE_SYSBOTTOM | XWIN_STYLE_ANTI_FSCR);

	grect_t desk;
	x.getDesktopSpace(desk, 0);
	desk.y += 28;
	desk.h -= 28;
	x.setDesktopSpace(desk);

	xwin.setTimer(1);
	x.run(NULL);
	return 0;
}

