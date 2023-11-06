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

using namespace Ewok;

class MenuItem : public Label {
protected:
	bool onMouse(xevent_t* ev) {
		if(ev->state == XEVT_MOUSE_DOWN)
			setFGColor(0xff888888);
		else if(ev->state == XEVT_MOUSE_UP)
			setFGColor(0xff000000);
		return true;
	}
public:
	MenuItem(font_t* font, const char* str): Label(font, str) { }
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
	
		wd = new MenuItem(X::getSysFont(), "EwokOS");
		wd->setMarginH(10);
		wd->fixedMinSize();
		container->add(wd);

		wd = new Widget();
		container->add(wd);
	}

	~Menubar() {
	}
};

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

	x.run(NULL);
	return 0;
}

